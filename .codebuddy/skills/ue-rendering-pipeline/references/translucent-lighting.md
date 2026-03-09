# UE5 半透明光照系统源码分析

> 基于 UE5.7 源码，源码位置：`E:\UE\UE_5.7\Engine\Source\Runtime\Renderer\Private\`

---

## 目录

1. [系统概述](#系统概述)
2. [Translucency Lighting Volume](#translucency-lighting-volume)
3. [Separate Translucency Pass](#separate-translucency-pass)
4. [Light Shaft / God Ray](#light-shaft--god-ray)
5. [OIT (Order Independent Transparency)](#oit-order-independent-transparency)
6. [Lumen Translucency](#lumen-translucency)
7. [Translucency Shadow Depth](#translucency-shadow-depth)
8. [渲染管线集成](#渲染管线集成)
9. [CVar 调试命令](#cvar-调试命令)

---

## 系统概述

UE 半透明光照系统采用**体积光照注入**方案，将场景光照信息注入到 3D 体积纹理中，半透明材质在渲染时采样该体积获取光照。

### 核心设计思想

```
光源 → 注入体积纹理 → 半透明材质采样 → 最终着色
```

### 源码文件结构

| 模块 | 源码文件 | 功能 |
|------|----------|------|
| 体积光照 | `TranslucentLighting.cpp/h` | 光照注入 3D Volume |
| 半透明渲染 | `TranslucentRendering.cpp/h` | Pass 管理、合成 |
| 光束效果 | `LightShaftRendering.cpp/h` | God Ray 实现 |
| OIT | `OIT/OIT.cpp/h` | 顺序无关透明 |
| Lumen 集成 | `Lumen/LumenTranslucencyVolumeLighting.cpp/h` | GI 支持 |
| 阴影深度 | `TranslucentLighting.cpp` (ShadowDepth 部分) | 半透明投射阴影 |

---

## Translucency Lighting Volume

### 核心原理

将光照注入到双级联 3D 体积纹理：
- **Inner Cascade**: 近距离高分辨率
- **Outer Cascade**: 远距离低分辨率

### 关键数据结构

```cpp
// TranslucentLighting.h:15-48
struct FTranslucencyLightingVolumeTextures
{
    TArray<FRDGTextureRef, TInlineAllocator<TVC_MAX>> Ambient;      // 环境光
    TArray<FRDGTextureRef, TInlineAllocator<TVC_MAX>> Directional;  // 方向光
    int32 VolumeDim = 0;                                            // 体积维度
    
    // 获取视图对应的纹理索引
    int32 GetIndex(const FViewInfo& View, int32 CascadeIndex) const;
    
    // 初始化并清除
    void Init(FRDGBuilder& GraphBuilder, TArrayView<const FViewInfo> Views, ERDGPassFlags PassFlags);
};
```

### 光照注入流程

```
1. CalcTranslucencyLightingVolumeBounds() - 计算体积边界
   └─ 基于 InnerDistance/OuterDistance 计算 Frustum 包围盒
   └─ 位置 Snap 到体素网格确保稳定性

2. FTranslucentLightInjectionCollector - 收集光照
   └─ AddLightForInjection() 添加光源
   └─ 支持批量注入优化 (r.TranslucencyLightingVolume.Batch)

3. 注入着色器
   └─ 方向光 + 阴影采样
   └─ 局部光源 (点光源、聚光灯)
   └─ 光函数材质支持
```

### 体积边界计算

```cpp
// TranslucentLighting.cpp:249-351
void FViewInfo::CalcTranslucencyLightingVolumeBounds(FBox* InOutCascadeBoundsArray, int32 NumCascades) const
{
    // CVar 距离配置
    double InnerDistance = CVarTranslucencyLightingVolumeInnerDistance.GetValueOnRenderThread();  // 默认 1500
    double OuterDistance = CVarTranslucencyLightingVolumeOuterDistance.GetValueOnRenderThread();  // 默认 5000
    
    // Frustum 顶点计算
    const double TanFieldOfView = FMath::Tan(FieldOfView);
    const double EndHorizontalLength = FrustumEndDistance * TanFieldOfView;
    
    // 构建包围球
    FSphere SphereBounds(Center, FMath::Sqrt(RadiusSquared));
    
    // Snap 到体素网格
    SphereBounds.Center.X -= FMath::Fmod(SphereBounds.Center.X, SphereBounds.W * 2 / VolumeDim);
    // ... Y, Z 同理
}
```

### Shader 参数传递

```cpp
// TranslucentLighting.h:50-57
BEGIN_SHADER_PARAMETER_STRUCT(FTranslucencyLightingVolumeParameters, )
    SHADER_PARAMETER_RDG_TEXTURE(Texture3D, TranslucencyLightingVolumeAmbientInner)
    SHADER_PARAMETER_RDG_TEXTURE(Texture3D, TranslucencyLightingVolumeAmbientOuter)
    SHADER_PARAMETER_RDG_TEXTURE(Texture3D, TranslucencyLightingVolumeDirectionalInner)
    SHADER_PARAMETER_RDG_TEXTURE(Texture3D, TranslucencyLightingVolumeDirectionalOuter)
    SHADER_PARAMETER(float, TranslucencyLightingRandomPositionOffsetRadius)
    SHADER_PARAMETER(float, TranslucencyLightingForwardUsingVolume)
END_SHADER_PARAMETER_STRUCT()
```

---

## Separate Translucency Pass

### Pass 类型

```cpp
// TranslucentRendering.cpp:126-135
static const TCHAR* kTranslucencyPassName[] = {
    TEXT("BeforeDistortion"),        // 畸变前
    TEXT("BeforeDistortionModulate"),// 畸变前调制
    TEXT("AfterDOF"),                // DOF 后
    TEXT("AfterDOFModulate"),        // DOF 后调制
    TEXT("AfterMotionBlur"),         // 运动模糊后
    TEXT("Holdout"),                 // Holdout
    TEXT("All"),                     // 全部
};
```

### Pass 到 MeshPass 映射

```cpp
// TranslucentRendering.cpp:164-182
EMeshPass::Type TranslucencyPassToMeshPass(ETranslucencyPass::Type TranslucencyPass)
{
    switch (TranslucencyPass)
    {
    case ETranslucencyPass::TPT_TranslucencyStandard:       
        return EMeshPass::TranslucencyStandard;
    case ETranslucencyPass::TPT_TranslucencyAfterDOF:       
        return EMeshPass::TranslucencyAfterDOF;
    // ...
    }
}
```

### 分辨率管理

```cpp
// TranslucentRendering.cpp:263-286
FSeparateTranslucencyDimensions UpdateSeparateTranslucencyDimensions(const FSceneRenderer& SceneRenderer)
{
    // 降采样百分比 (默认 100%)
    float TranslucencyResolutionFraction = CVarSeparateTranslucencyScreenPercentage / 100.0f;
    
    // 动态分辨率支持
    if (GDynamicTranslucencyResolution.GetSettings().IsEnabled())
    {
        TranslucencyResolutionFraction = SceneRenderer.DynamicResolutionFractions[GDynamicTranslucencyResolution];
    }
    
    Dimensions.Extent = GetScaledExtent(SceneRenderer.ViewFamily.SceneTexturesConfig.Extent, TranslucencyResolutionFraction);
    Dimensions.Scale = TranslucencyResolutionFraction;
    return Dimensions;
}
```

### 合成操作

```cpp
// TranslucentRendering.h:64-90
struct FTranslucencyComposition
{
    enum class EOperation
    {
        UpscaleOnly,                  // 仅放大
        ComposeToExistingSceneColor,  // 合成到已有 SceneColor
        ComposeToNewSceneColor,       // 合成到新 SceneColor
        ComposeToSceneColorAlpha      // 合成到 Alpha 通道
    };
    
    EOperation Operation;
    bool bApplyModulateOnly = false;
    FScreenPassTextureSlice SceneColor;
    FScreenPassTexture SceneDepth;
    
    FScreenPassTexture AddPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, 
                               const FTranslucencyPassResources& TranslucencyTextures) const;
};
```

---

## Light Shaft / God Ray

### 核心实现

```cpp
// LightShaftRendering.cpp:16-23
int32 GLightShafts = 1;                     // 启用开关
int32 GLightShaftDownsampleFactor = 2;      // 降采样因子
int32 GLightShaftBlurPasses = 3;            // 模糊 Pass 数量
int32 GLightShaftBlurNumSamples = 12;       // 每次模糊采样数
float GLightShaftFirstPassDistance = .1f;   // 首次模糊距离比例
```

### 技术类型

```cpp
// LightShaftRendering.cpp:130-134
enum class ELightShaftTechnique
{
    Occlusion,  // 遮挡遮罩
    Bloom       // Bloom 效果
};
```

### 输出目标

```cpp
// LightShaftRendering.cpp:97-106
enum class ELightShaftBloomOutput
{
    SceneColor,           // 直接输出到 SceneColor
    SeparateTranslucency  // 输出到 SeparateTranslucency (DOF 后)
};

ELightShaftBloomOutput GetLightShaftBloomOutput(const FSceneViewFamily& ViewFamily)
{
    return (ViewFamily.AllowTranslucencyAfterDOF() && GLightShaftRenderAfterDOF) 
           ? ELightShaftBloomOutput::SeparateTranslucency 
           : ELightShaftBloomOutput::SceneColor;
}
```

### 着色器参数

```cpp
// LightShaftRendering.cpp:136-144
BEGIN_SHADER_PARAMETER_STRUCT(FLightShaftPixelShaderParameters, )
    SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
    SHADER_PARAMETER(FVector4f, UVMinMax)
    SHADER_PARAMETER(FVector4f, AspectRatioAndInvAspectRatio)
    SHADER_PARAMETER(FVector4f, LightShaftParameters)
    SHADER_PARAMETER(FVector4f, BloomTintAndThreshold)
    SHADER_PARAMETER(FVector2f, TextureSpaceBlurOrigin)
    SHADER_PARAMETER(float, BloomMaxBrightness)
END_SHADER_PARAMETER_STRUCT()
```

---

## OIT (Order Independent Transparency)

### Pass 类型

```cpp
// OIT/OIT.h:16-21
enum EOITPassType
{
    OITPass_None = 0,
    OITPass_RegularTranslucency = 1,   // 常规半透明
    OITPass_SeperateTranslucency = 2   // 分离半透明
};
```

### 三角形排序

```cpp
// OIT/OIT.h:23-27
enum class FTriangleSortingOrder
{
    FrontToBack,   // 前到后
    BackToFront,   // 后到前
};
```

### OIT 数据结构

```cpp
// OIT/OIT.h:50-61
struct FOITData
{
    EOITPassType PassType = OITPass_None;
    uint32 SupportedPass = 0;
    uint32 Method = 0;
    uint32 MaxSamplePerPixel = 0;
    uint32 MaxSideSamplePerPixel = 0;
    float TransmittanceThreshold = 0.f;

    FRDGTextureRef SampleDataTexture = nullptr;    // 样本数据
    FRDGTextureRef SampleCountTexture = nullptr;   // 样本计数
};
```

### API 接口

```cpp
// OIT/OIT.h:79-109
namespace OIT
{
    // 三角形排序
    bool IsSortedTrianglesEnabled(EShaderPlatform InPlatform);
    void AddSortTrianglesPass(FRDGBuilder& GraphBuilder, const FViewInfo& View, 
                              FOITSceneData& OITSceneData, FTriangleSortingOrder SortType);
    
    // 像素排序
    bool IsSortedPixelsEnabled(const FViewInfo& InView);
    bool IsSortedPixelsEnabledForPass(EOITPassType PassType);
    
    // 创建和合成
    FOITData CreateOITData(FRDGBuilder& GraphBuilder, const FViewInfo& View, EOITPassType PassType);
    void AddOITComposePass(FRDGBuilder& GraphBuilder, const FViewInfo& View, 
                           FOITData& OITData, FRDGTextureRef SceneColorTexture);
}
```

---

## Lumen Translucency

### GI 体积结构

```cpp
// LumenTranslucencyVolumeLighting.h:19-30
class FLumenTranslucencyGIVolume
{
public:
    LumenRadianceCache::FRadianceCacheInterpolationParameters RadianceCacheInterpolationParameters;
    FRDGTextureRef Texture0 = nullptr;        // 当前帧体积 0
    FRDGTextureRef Texture1 = nullptr;        // 当前帧体积 1
    FRDGTextureRef HistoryTexture0 = nullptr; // 历史体积 0
    FRDGTextureRef HistoryTexture1 = nullptr; // 历史体积 1
    FVector GridZParams = FVector::ZeroVector;
    uint32 GridPixelSizeShift = 0;
    FIntVector GridSize = FIntVector::ZeroValue;
};
```

### 着色器参数

```cpp
// LumenTranslucencyVolumeLighting.h:33-44
BEGIN_SHADER_PARAMETER_STRUCT(FLumenTranslucencyLightingParameters, )
    SHADER_PARAMETER_STRUCT_INCLUDE(LumenRadianceCache::FRadianceCacheInterpolationParameters, RadianceCacheInterpolationParameters)
    SHADER_PARAMETER_STRUCT_INCLUDE(FLumenFrontLayerTranslucencyReflectionParameters, FrontLayerTranslucencyReflectionParameters)
    SHADER_PARAMETER_RDG_TEXTURE(Texture3D, TranslucencyGIVolume0)
    SHADER_PARAMETER_RDG_TEXTURE(Texture3D, TranslucencyGIVolume1)
    SHADER_PARAMETER_RDG_TEXTURE(Texture3D, TranslucencyGIVolumeHistory0)
    SHADER_PARAMETER_RDG_TEXTURE(Texture3D, TranslucencyGIVolumeHistory1)
    SHADER_PARAMETER_SAMPLER(SamplerState, TranslucencyGIVolumeSampler)
    SHADER_PARAMETER(FVector3f, TranslucencyGIGridZParams)
    SHADER_PARAMETER(uint32, TranslucencyGIGridPixelSizeShift)
    SHADER_PARAMETER(FIntVector, TranslucencyGIGridSize)
END_SHADER_PARAMETER_STRUCT()
```

### 光线追踪接口

```cpp
// LumenTranslucencyVolumeLighting.h:86-96
extern void HardwareRayTraceTranslucencyVolume(
    FRDGBuilder& GraphBuilder,
    const FViewInfo& View,
    const FLumenCardTracingParameters& TracingParameters,
    LumenRadianceCache::FRadianceCacheInterpolationParameters RadianceCacheParameters,
    FLumenTranslucencyLightingVolumeParameters VolumeParameters,
    FLumenTranslucencyLightingVolumeTraceSetupParameters TraceSetupParameters,
    FRDGTextureRef VolumeTraceRadiance,
    FRDGTextureRef VolumeTraceHitDistance,
    ERDGPassFlags ComputePassFlags
);
```

### Ray Traced Translucency

```cpp
// RayTracedTranslucency.h:7-17
namespace RayTracedTranslucency
{
    bool IsEnabled(const FViewInfo& View);
    bool UseForceOpaque();
    bool UseRayTracedRefraction(const TArray<FViewInfo>& Views);
    bool AllowTranslucentReflectionInReflections();
    float GetPathThroughputThreshold();
    uint32 GetDownsampleFactor(const TArray<FViewInfo>& Views);
    uint32 GetMaxPrimaryHitEvents(const FViewInfo& View);
    uint32 GetMaxSecondaryHitEvents(const FViewInfo& View);
}
```

---

## Translucency Shadow Depth

### 着色器模式

```cpp
// TranslucentLighting.cpp:410-414
enum ETranslucencyShadowDepthShaderMode
{
    TranslucencyShadowDepth_PerspectiveCorrect,  // 透视正确
    TranslucencyShadowDepth_Standard,            // 标准
};
```

### Mesh Processor

```cpp
// TranslucentLighting.cpp:497-532
class FTranslucencyDepthPassMeshProcessor : public FMeshPassProcessor
{
    // 处理半透明网格绘制
    bool TryAddMeshBatch(const FMeshBatch& MeshBatch, ...);
    
    // 根据光源类型选择着色器模式
    // 方向光: Standard
    // 其他光源: PerspectiveCorrect
    template<ETranslucencyShadowDepthShaderMode ShaderMode>
    bool Process(const FMeshBatch& MeshBatch, ...);
    
private:
    FMeshPassProcessorRenderState PassDrawRenderState;
    const FProjectedShadowInfo* ShadowInfo;
    const bool bDirectionalLight;
};
```

### 渲染入口

```cpp
// TranslucentLighting.cpp:667-698
void FProjectedShadowInfo::RenderTranslucencyDepths(
    FRDGBuilder& GraphBuilder, 
    FSceneRenderer* SceneRenderer, 
    const FRenderTargetBindingSlots& InRenderTargets, 
    FInstanceCullingManager& InstanceCullingManager)
{
    // 设置混合状态：Additive 累积
    DrawRenderState.SetBlendState(TStaticBlendState<
        CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One,
        CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());
    
    // 创建 Mesh Processor
    FTranslucencyDepthPassMeshProcessor TranslucencyDepthPassMeshProcessor(
        SceneRenderer->Scene, ShadowDepthView, DrawRenderState, this, ...);
}
```

---

## 渲染管线集成

### DeferredShadingRenderer 调用流程

```
Render() 
└─ InitViews()
│  └─ CalcTranslucencyLightingVolumeBounds()
│
└─ RenderTranslucencyVolumeLighting()
│  └─ 注入光照到 Volume Texture
│
└─ RenderBasePass()
│  └─ 半透明材质采样 Volume
│
└─ RenderTranslucency()
│  ├─ BeforeDistortion
│  ├─ AfterDOF (Separate)
│  └─ AfterMotionBlur
│
└─ RenderLightShafts()
   └─ 径向模糊
```

### 关键函数位置

| 函数 | 文件 | 行号范围 |
|------|------|----------|
| `CalcTranslucencyLightingVolumeBounds` | TranslucentLighting.cpp | 249-351 |
| `RenderTranslucencyDepths` | TranslucentLighting.cpp | 667+ |
| `UpdateSeparateTranslucencyDimensions` | TranslucentRendering.cpp | 263-286 |
| `TranslucencyPassToMeshPass` | TranslucentRendering.cpp | 164-182 |
| `GetMobileLightShaftInfo` | LightShaftRendering.cpp | 81-95 |

---

## CVar 调试命令

### 体积光照

| CVar | 默认值 | 说明 |
|------|--------|------|
| `r.TranslucencyLightingVolume` | 1 | 启用体积光照 |
| `r.TranslucencyLightingVolume.Dim` | 64 | 体积分辨率 |
| `r.TranslucencyLightingVolume.InnerDistance` | 1500 | 内层距离 |
| `r.TranslucencyLightingVolume.OuterDistance` | 5000 | 外层距离 |
| `r.TranslucencyLightingVolume.Blur` | 1 | 模糊 |
| `r.TranslucencyLightingVolume.Temporal` | false | 时域累积 |
| `r.TranslucencyLightingVolume.Visualize` | 0 | 可视化 |
| `r.TranslucencyLightingVolume.Debug` | 0 | 调试信息 |

### 分离半透明

| CVar | 默认值 | 说明 |
|------|--------|------|
| `r.SeparateTranslucencyScreenPercentage` | 100 | 分辨率百分比 |
| `r.SeparateTranslucencyUpsampleMode` | 1 | 放大模式 |
| `r.Translucency.DynamicRes.TimeBudget` | - | 动态分辨率时间预算 |

### 光束

| CVar | 默认值 | 说明 |
|------|--------|------|
| `r.LightShaftQuality` | 1 | 质量 |
| `r.LightShaftDownSampleFactor` | 2 | 降采样因子 |
| `r.LightShaftBlurPasses` | 3 | 模糊 Pass 数 |
| `r.LightShaftNumSamples` | 12 | 采样数 |
| `r.LightShaftRenderToSeparateTranslucency` | 0 | 渲染到分离半透明 |

### OIT

| CVar | 默认值 | 说明 |
|------|--------|------|
| `r.OIT.SortedPixels` | - | 像素排序 |
| `r.OIT.SortedTriangles` | - | 三角形排序 |

---

## 调试技巧

### 可视化体积光照

```bash
# 启用可视化
r.TranslucencyLightingVolume.Visualize 1

# 选择级联
r.TranslucencyLightingVolume.VisualizeCascadeIndex 0  # Inner
r.TranslucencyLightingVolume.VisualizeCascadeIndex 1  # Outer
r.TranslucencyLightingVolume.VisualizeCascadeIndex -1 # 全部

# 调整可视化球体大小
r.TranslucencyLightingVolume.Visualize.RadiusScale 2.0
```

### 冻结体积位置

```bash
# 冻结体积位置便于调试
r.TranslucencyLightingVolume.Debug.FreezePosition 1
r.TranslucencyLightingVolume.Debug 1
```

### 性能分析

```bash
# 降低体积分辨率提升性能
r.TranslucencyLightingVolume.Dim 32

# 禁用模糊
r.TranslucencyLightingVolume.Blur 0

# 使用降采样分离半透明
r.SeparateTranslucencyScreenPercentage 50
```

---

## 半透明光照模式详解

### 光照模式分类

UE 半透明材质支持三种主要光照模式，通过材质属性 `Translucency Lighting Mode` 控制：

| 模式 | 宏定义 | 特点 | 适用场景 |
|------|--------|------|----------|
| **Volumetric PerVertex Directional** | `TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_DIRECTIONAL` | 顶点采样 Volume + 像素应用方向 | 性能优先，大量粒子 |
| **Volumetric PerVertex NonDirectional** | `TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_NONDIRECTIONAL` | 仅顶点环境光 | 极低开销，远景 |
| **Surface LightingVolume** | `TRANSLUCENCY_LIGHTING_SURFACE_LIGHTINGVOLUME` | 像素采样 Volume + 完整光照 | 平衡质量与性能 |
| **Surface ForwardShading** | `TRANSLUCENCY_LIGHTING_SURFACE_FORWARDSHADING` | 前向着色，逐光源计算 | 高质量，少量光源 |

### 模式 1: Volumetric PerVertex Directional

```hlsl
// BasePassVertexShader.usf:148-157
#if TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_DIRECTIONAL
    // 顶点着色器中采样体积纹理
    Output.BasePassInterpolants.AmbientLightingVector = GetAmbientLightingVectorFromTranslucentLightingVolume(
        InnerVolumeUVs, OuterVolumeUVs, FinalLerpFactor).xyz;
    Output.BasePassInterpolants.DirectionalLightingVector = GetDirectionalLightingVectorFromTranslucentLightingVolume(
        InnerVolumeUVs, OuterVolumeUVs, FinalLerpFactor);
#endif
```

**优点：**
- 顶点级计算，GPU 开销极低
- 适合大量粒子系统

**缺点：**
- 顶点密度不足时光照变化不平滑
- 无法响应像素级法线变化

### 模式 2: Surface LightingVolume (默认推荐)

```hlsl
// BasePassPixelShader.usf:201-204
#if TRANSLUCENCY_LIGHTING_SURFACE_LIGHTINGVOLUME
#include "ForwardLightingCommon.ush"
#endif
```

**光照计算流程：**
```
1. 像素着色器采样 TranslucencyLightingVolume
   └─ GetAmbientLightingVectorFromTranslucentLightingVolume()
   └─ GetDirectionalLightingVectorFromTranslucentLightingVolume()

2. 重构球谐系数
   └─ ReconstructSHCoefficients(Ambient, Directional)

3. 应用材质法线
   └─ GetVolumeLightingDirectional() 带法线计算

4. 叠加直接光照 (如有动态光源)
```

**性能开销：**
- 2 次 3D 纹理采样 (Inner/Outer Cascade)
- 1 次线性插值
- 球谐光照计算

### 模式 3: Surface ForwardShading

前向着色路径，逐光源循环计算。

**优点：**
- 完整的逐像素光照
- 支持阴影、光函数、IES
- 高光反射正确

**缺点：**
- 光源数量线性影响开销
- 需要光栅化阴影贴图

---

## ShadingModel 对半透明表现的影响

### ShadingModel 列表

```cpp
// EngineTypes.h:704-725
enum EMaterialShadingModel : int
{
    MSM_Unlit,              // 无光照
    MSM_DefaultLit,         // 默认 PBR
    MSM_Subsurface,         // 次表面散射
    MSM_PreintegratedSkin,  // 预积分皮肤
    MSM_ClearCoat,          // 清漆
    MSM_SubsurfaceProfile,  // 次表面配置
    MSM_TwoSidedFoliage,    // 双面植被
    MSM_Hair,               // 头发
    MSM_Cloth,              // 布料
    MSM_Eye,                // 眼睛
    MSM_SingleLayerWater,   // 单层水
    MSM_ThinTranslucent,    // 薄半透明
    MSM_Strata,             // Substrate (隐藏)
};
```

### Lit vs Unlit 性能差异

```cpp
// BasePassRendering.cpp:2335-2336
const bool bIsLitMaterial = ShadingModels.IsLit();
const bool bAllowStaticLighting = IsStaticLightingAllowed();
```

**IsLit() 判断：**
- `MSM_Unlit` → 返回 false
- 所有其他 ShadingModel → 返回 true

**性能影响：**

| ShadingModel | Lit 状态 | 额外开销 |
|--------------|----------|----------|
| Unlit | false | 仅 Emissive，无光照计算 |
| DefaultLit | true | Volume 采样 + PBR 光照 |
| Subsurface | true | + 次表面散射计算 |
| ThinTranslucent | true | + 双源混合 + 透射计算 |

### MSM_ThinTranslucent 详解

专门为半透明优化的 ShadingModel，使用 Dual Source Blending：

```cpp
// BasePassRendering.cpp:300-310
else if (Material.GetShadingModels().HasShadingModel(MSM_ThinTranslucent))
{
    if (Material.IsDualBlendingEnabled())
    {
        // Dual Blending: Add + Multiply
        DrawRenderState.SetBlendState(TStaticBlendState<
            CW_RGBA, BO_Add, BF_One, BF_Source1Color, 
            BO_Add, BF_One, BF_Source1Alpha>::GetRHI());
    }
}
```

**核心算法 (ThinTranslucentCommon.ush):**

```hlsl
void AccumulateThinTranslucentModel(
    inout float3 DualBlendSurfaceLuminancePostCoverage,   // Add 目标
    inout float3 DualBlendSurfaceTransmittancePreCoverage, // Mul 目标
    inout float DualBlendSurfaceCoverage,
    ...)
{
    const float NoV = saturate(abs(dot(N, V)) + 1e-5);
    
    // 透射颜色 (吸收)
    float3 TransmittanceColor = GetThinTranslucentMaterialOutput0(MaterialParams);
    
    // 厚度基于观察角度: 1/cos(θ)
    float PathLength = rcp(NoV);
    float3 NegativeAbsorptionCoefficient = log(TransmittanceColor);
    float3 BottomMaterialTransmittance = exp(NegativeAbsorptionCoefficient * PathLength);
    
    // Fresnel 透射 (进出两次)
    const float3 FresnelRatio = F_Schlick(GBuffer.SpecularColor, NoV);
    Transmittance = Transmittance * (1 - FresnelRatio);
    Transmittance = Transmittance * BottomMaterialTransmittance;
    Transmittance = Transmittance * (1 - FresnelRatio);
    
    // 双源混合输出
    DualBlendSurfaceLuminancePostCoverage = SurfaceColor * DualBlendSurfaceCoverage;
    DualBlendSurfaceTransmittancePreCoverage = Transmittance;
}
```

**表现特点：**
- 物理正确的透射
- 基于视角的厚度变化
- 正确的折射 Fresnel
- 单 Pass 双源混合

### Subsurface 相关 ShadingModel

```cpp
// MaterialShared.h:127-130
inline bool UsesSubsurfaceProfile(FMaterialShadingModelField ShadingModel)
{
    return ShadingModel.HasShadingModel(MSM_Subsurface) ||
           ShadingModel.HasShadingModel(MSM_PreintegratedSkin) ||
           ShadingModel.HasShadingModel(MSM_SubsurfaceProfile) ||
           ShadingModel.HasShadingModel(MSM_TwoSidedFoliage) ||
           ShadingModel.HasShadingModel(MSM_Cloth) ||
           ShadingModel.HasShadingModel(MSM_Eye);
}
```

**性能影响：**
- 需要额外的 Subsurface Profile 查找
- 需要后处理 Pass (屏幕空间 SSS)
- 半透明时使用体积阴影投射

---

## ShadingModel 性能对比

### GPU 开销估算

| ShadingModel | 相对开销 | 推荐场景 |
|--------------|----------|----------|
| **Unlit** | 1x (基准) | 特效、UI、自发光物体 |
| **DefaultLit** | 2-3x | 通用半透明 |
| **ThinTranslucent** | 2.5-3x | 玻璃、薄片、肥皂泡 |
| **Subsurface** | 3-4x | 皮肤、蜡、玉石 |
| **SubsurfaceProfile** | 4-5x | 高质量皮肤 |
| **Hair** | 4-5x | 头发、毛发 |
| **SingleLayerWater** | 3-4x | 水面 |

### 材质设置建议

**高性能粒子：**
```
Lighting Mode: Volumetric PerVertex NonDirectional
Shading Model: Unlit 或 DefaultLit
Blend Mode: Additive 或 Translucent
```

**高质量玻璃：**
```
Lighting Mode: Surface LightingVolume
Shading Model: ThinTranslucent
Blend Mode: Translucent
启用 Dual Source Blending
```

**角色皮肤：**
```
Lighting Mode: Surface ForwardShading
Shading Model: SubsurfaceProfile
配置 Subsurface Profile 资产
```

---

## Billboard 半透明雾片光照问题解决方案

### 问题根因

**场景描述：** 永远面向相机的半透明雾片（Billboard），添加法线后背光处很黑，无法反映体积透光感。

**根因分析：**
```
Billboard 始终面向相机 → 法线 N 始终指向相机方向
背光时：光源在相机后方 → N·L < 0
DefaultLit 的 Diffuse 项 = max(0, N·L) = 0 → 背光处全黑
```

即使有法线贴图，也只是改变高光方向，**不会产生体积透光感**。

---

### 解决方案对比

| 方案 | 效果 | 性能 | 复杂度 | 推荐场景 |
|------|------|------|--------|----------|
| **TwoSidedFoliage** | ★★★★★ | ★★★★☆ | 低 | 雾、烟、植被、体积效果 |
| **Subsurface** | ★★★★☆ | ★★★☆☆ | 低 | 雾、云、半透明物体 |
| **ThinTranslucent** | ★★★★☆ | ★★★☆☆ | 中 | 玻璃、薄片 |
| **自定义材质** | ★★★☆☆ | ★★★★☆ | 高 | 特殊需求 |

---

### 方案 1: TwoSidedFoliage ShadingModel（推荐）

**最适合 Billboard 雾片**，专门处理双面透光。

---

#### BaseColor 与 SubsurfaceColor 的关系

**核心概念：**
- **BaseColor** → 前向光照颜色（N·L > 0 时）
- **SubsurfaceColor** → 背光透射颜色（N·L < 0 时）

**光照计算分离：**

```hlsl
// ShadingModels.ush:941-951
FDirectLighting TwoSidedBxDF(FGBufferData GBuffer, half3 N, half3 V, ...)
{
    // 1. 前向光照：使用 BaseColor (GBuffer.DiffuseColor)
    FDirectLighting Lighting = DefaultLitBxDF(GBuffer, N, V, AreaLight, Shadow);
    //    └─ Diffuse = BaseColor * N·L
    //    └─ Specular = 高光计算

    // 2. 背光透射：使用 SubsurfaceColor
    half WrapNoL = saturate((-dot(N, L) + 0.5) / 2.25);
    half VoL = dot(V, L);
    float Scatter = D_GGX(0.36, saturate(-VoL));
    
    // 3. Transmission 项：仅使用 SubsurfaceColor
    Lighting.Transmission = WrapNoL * Scatter * SubsurfaceColor;
    
    return Lighting;
}
```

**最终颜色合成：**
```
最终颜色 = Diffuse(前向) + Specular + Transmission(背光)

前向光照时：Diffuse 主导，颜色 = BaseColor
背光时：Transmission 主导，颜色 = SubsurfaceColor
```

---

#### 可视化理解

```
        光源 L
          ↓
    ┌─────────────┐
    │             │
    │   雾片      │ ← 法线 N 指向相机
    │             │
    └─────────────┘
          ↑
        相机 V

情况1：光源在相机前方 (N·L > 0)
├─ 前向光照激活
├─ Diffuse = BaseColor * N·L
└─ Transmission ≈ 0

情况2：光源在相机后方 (N·L < 0) ← 背光！
├─ 前向光照 ≈ 0 (BaseColor 不显示)
├─ Transmission 激活
└─ 透射颜色 = SubsurfaceColor
```

---

#### 材质设置建议

**雾片/烟雾：**
```
BaseColor: 雾的基础颜色（前向显示）
           - 白色/灰色：普通雾
           - 深色：烟尘

Subsurface Color: 透射颜色（背光显示）
                  - 暖色（橙黄）：阳光穿透效果
                  - 冷色（蓝紫）：月光/氛围效果
                  - 高亮度：强透光感

Opacity: 0.3-0.7（控制整体透明度）
```

**能量关系：**
- BaseColor 和 SubsurfaceColor **独立计算**，不会相加
- 设置时可以：
  - BaseColor 暗 + SubsurfaceColor 亮 = 背光时更亮
  - BaseColor 亮 + SubsurfaceColor 暗 = 前向时更亮
  - 两者相近 = 均匀透光

---

### 方案 2: Subsurface ShadingModel

适合需要次表面散射效果的物体（蜡、玉石、皮肤等）。

---

#### BaseColor 与 SubsurfaceColor 的关系

**核心区别：与 TwoSidedFoliage 不同！**

| ShadingModel | BaseColor + SubsurfaceColor | 关系 |
|--------------|----------------------------|------|
| **TwoSidedFoliage** | 独立计算 | 前向用 BaseColor，背光用 SubsurfaceColor |
| **Subsurface** | **相加** | DiffuseColor = BaseColor + SubsurfaceColor |

---

#### 源码分析

**直接光照（SubsurfaceBxDF，ShadingModels.ush:717-751）：**
```hlsl
FDirectLighting SubsurfaceBxDF(FGBufferData GBuffer, half3 N, half3 V, ...)
{
    // 1. 基础光照：使用 BaseColor
    FDirectLighting Lighting = DefaultLitBxDF(GBuffer, N, V, AreaLight, Shadow);
    //    └─ Diffuse = BaseColor * N·L

    // 2. 获取 SubsurfaceColor 和 Opacity
    half3 SubsurfaceColor = ExtractSubsurfaceColor(GBuffer);
    half Opacity = GBuffer.CustomData.a;

    // 3. InScatter: 视角与光源方向相反时的散射（背光效果）
    //    当 V 与 -L 对齐时（背光看物体），InScatter 最大
    half InScatter = pow(saturate(dot(L, -V)), 12) * lerp(3, 0.1, Opacity);

    // 4. WrappedDiffuse: Wrap Lighting（让光照更柔和）
    half WrappedDiffuse = pow(saturate(N·L * 0.667 + 0.333), 1.5) * 1.667;
    half NormalContribution = lerp(1, WrappedDiffuse, Opacity);
    half BackScatter = GBuffer.GBufferAO * NormalContribution / (PI * 2);

    // 5. Transmission: 使用 Beer-Lambert 吸收模型
    //    模拟光在材质内部的衰减
    half3 ExtinctionCoefficients = TransmittanceToExtinction(SubsurfaceColor, ...);
    half3 TransmittedColor = ExtinctionToTransmittance(ExtinctionCoefficients, 1.0);

    // 6. 最终 Transmission
    Lighting.Transmission = Falloff * lerp(BackScatter, 1, InScatter) 
                          * lerp(TransmittedColor, SubsurfaceColor, Thickness);

    return Lighting;
}
```

**间接光照（DiffuseIndirectComposite.usf:580-584）：**
```hlsl
// 关键：Subsurface ShadingModel 会把 SubsurfaceColor 加到 DiffuseColor
if (GBuffer.ShadingModelID == SHADINGMODELID_SUBSURFACE || 
    GBuffer.ShadingModelID == SHADINGMODELID_PREINTEGRATED_SKIN)
{
    float3 SubsurfaceColor = ExtractSubsurfaceColor(GBuffer);
    // Add subsurface energy to diffuse
    DiffuseColor += SubsurfaceColor;  // 相加！影响间接光照
}

// 间接光照使用的是 (BaseColor + SubsurfaceColor)
IndirectLighting.Diffuse = DiffuseIndirectLighting * DiffuseColor;
```

---

#### 最终颜色合成

```
直接光照：
├─ Diffuse = BaseColor * N·L（DefaultLitBxDF）
├─ Specular = 高光计算
└─ Transmission = InScatter * SubsurfaceColor（背光效果）

间接光照：
└─ Diffuse = (BaseColor + SubsurfaceColor) * IndirectLighting

最终颜色 = Diffuse + Specular + Transmission + 间接光照
```

---

#### 与 TwoSidedFoliage 对比

| 特性 | TwoSidedFoliage | Subsurface |
|------|-----------------|------------|
| **BaseColor 用途** | 仅前向光照 | 前向光照 + 间接光照 |
| **SubsurfaceColor 用途** | 仅背光透射 | 背光透射 + **叠加到间接光照** |
| **颜色关系** | 独立 | **相加** |
| **间接光照** | 不受 SubsurfaceColor 影响 | 受 SubsurfaceColor 影响 |
| **适用场景** | 雾片、植被 | 蜡、玉石、半透明物体 |

---

#### 材质设置建议

**半透明物体（蜡、玉石）：**
```
BaseColor: 物体的基础颜色
Subsurface Color: 内部散射颜色
           - 皮肤：红色（血液散射）
           - 蜡：黄色/橙色
           - 玉石：绿色/青色
Opacity: 控制散射强度
         - 高 Opacity = 更不透明，散射弱
         - 低 Opacity = 更透明，散射强
```

**注意：**
- SubsurfaceColor 会**增强间接光照**
- 如果不需要间接光照中显示 SubsurfaceColor，使用 TwoSidedFoliage
- Subsurface 更适合需要"内部发光"效果的物体

---

### 方案 3: 自定义材质逻辑（DefaultLit）

如果必须使用 DefaultLit，可以在材质中添加自定义透射计算。

**材质节点网络：**

```
[Light Vector] ────┐
                   │
[Normal] ──────────┼─── [Dot] ──── [Abs] ──── [Saturate] ────┐
                   │                                         │
                   └─── [Negate] ──── [Saturate] ──┬─── [Lerp] ──── [Emissive]
                                                 │
[Subsurface Color] ───────────────────────────────┘

// 核心逻辑：
// Transmission = saturate(-N·L) * SubsurfaceColor
// Emissive += Transmission * LightIntensity
```

**更精确的实现：**

```
// 自定义材质函数 - 模拟 TwoSidedFoliage 的透射
float NdotL = dot(Normal, LightVector);
float WrapNdotL = saturate((-NdotL + 0.5) / 2.25);  // Wrap Lighting
float VoL = dot(CameraVector, LightVector);
float Scatter = D_GGX(0.36, saturate(-VoL));        // 散射分布
float3 Transmission = WrapNdotL * Scatter * SubsurfaceColor;
```

**注意：** 需要在材质中获取 LightVector 和 CameraVector，可能需要 Custom 节点。

---

### Volumetric PerVertex Directional 模式分析（关键！）

**问题：正面视角下 DefaultLit 和 TwoSidedFoliage 表现不同**

**根因：PerVertex 模式下 TwoSidedFoliage 的 Transmission 不计算！**

**顶点着色器（BasePassVertexShader.usf:148-175）：**
```hlsl
#if TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_DIRECTIONAL
    // 使用顶点法线 TangentToWorld[2]（不是材质法线！）
    Output.BasePassInterpolants.VertexDiffuseLighting = GetVolumeLightingDirectional(
        AmbientLightingVector, 
        DirectionalLightingVector, 
        VertexParameters.TangentToWorld[2],  // 顶点法线！
        GetMaterialTranslucencyDirectionalLightingIntensity()
    ).rgb;
#endif
```

**像素着色器（BasePassPixelShader.usf:276）：**
```hlsl
#if TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_DIRECTIONAL
    GetVolumeLightingDirectional(
        float4(BasePassInterpolants.AmbientLightingVector, 1), 
        BasePassInterpolants.DirectionalLightingVector, 
        MaterialParameters.WorldNormal,  // 材质法线
        GBuffer.DiffuseColor, 
        GetMaterialTranslucencyDirectionalLightingIntensity(), 
        DiffuseLighting, 
        VolumeLighting
    );
#endif
```

**关键发现：**

| 阶段 | 使用的法线 | 影响因素 |
|------|----------|--------|
| **顶点着色器** | 顶点法线 `TangentToWorld[2]` | 几何法线，不受法线贴图影响 |
| **像素着色器** | 材质法线 `WorldNormal` | 受法线贴图影响 |

**为什么正面视角下表现不同？**

1. **法线差异**：如果材质有法线贴图，材质法线 ≠ 顶点法线
2. **Transmission 缺失**：PerVertex 模式**不计算** TwoSidedFoliage 的 Transmission 项
   - TwoSidedFoliage 的 Transmission 在背光时**增加**亮度
   - PerVertex 模式下缺少这个补偿→ 背光时比 DefaultLit **更暗**

**解决方案：**

| 方案 | Transmission | 背光效果 | 适用场景 |
|------|--------------|----------|----------|
| **Volumetric PerVertex Directional** | ❌ 不计算 | 缺失，会更暗 | 大量粒子，性能优先 |
| **Surface LightingVolume** | ✅ 计算 | 正常 | 需要透射效果 |
| **Surface ForwardShading** | ✅ 计算 | 正常 | 高质量，少量光源 |

**PerVertex 模式下改善背光效果：**

如果必须使用 PerVertex 模式，可以：
- 降低 **Translucency Directional Lighting Intensity**（如 0.5）减少法线对光照的影响
- 增加 **Ambient Occlusion** 来补偿暗部
- 添加 **Emissive** 来补偿暗部

---

### 方案 4: 结合 TranslucencyDirectionalLightingIntensity

UE 提供了材质属性 `Translucency Directional Lighting Intensity`，控制体积光照的方向性：

**源码位置（TranslucencyVolumeCommon.ush:79）：**
```hlsl
FTwoBandSHVector DiffuseTransferSH = CalcDiffuseTransferSH(WorldNormal * DirectionalLightingIntensity, 1);
```

**设置：**
- 值 = 0：完全无方向性，类似环境光
- 值 = 1：正常方向性光照

**对雾片的影响：**
- 降低此值可以让背光处不那么黑
- 但无法产生真正的透射效果

---

### 最佳实践总结

**雾片/烟雾 Billboard：**
```
Shading Model: TwoSidedFoliage
Lighting Mode: Surface LightingVolume
Subsurface Color: 暖色调（橙黄），模拟光穿透
Opacity: 0.3-0.6
Two Sided: 启用
```

**大面积体积雾：**
```
Shading Model: Subsurface
Lighting Mode: Volumetric PerVertex NonDirectional
Subsurface Color: 根据雾的颜色设置
```

**性能优化：**
- 远景雾片使用 `Volumetric PerVertex NonDirectional`
- 近景雾片使用 `Surface LightingVolume`
- 避免使用 `Surface ForwardShading`（开销大）

---

## 源码位置速查表

| 功能模块 | 文件路径 |
|----------|----------|
| 体积光照注入 | `Renderer/Private/TranslucentLighting.cpp/h` |
| 半透明渲染 Pass | `Renderer/Private/TranslucentRendering.cpp/h` |
| Pass 资源定义 | `Renderer/Internal/TranslucentPassResource.h` |
| 光束效果 | `Renderer/Private/LightShaftRendering.cpp/h` |
| OIT 实现 | `Renderer/Private/OIT/OIT.cpp/h` |
| Lumen 半透明 | `Renderer/Private/Lumen/LumenTranslucencyVolumeLighting.cpp/h` |
| 光线追踪半透明 | `Renderer/Private/Lumen/RayTracedTranslucency.h` |
| 前层半透明 | `Renderer/Private/Lumen/LumenFrontLayerTranslucency.cpp` |
| 移动端半透明 | `Renderer/Private/MobileSeparateTranslucencyPass.cpp` |
| 着色器源码 | `Engine/Shaders/Private/TranslucentShadowDepthShaders.usf` |
| Thin Translucent | `Engine/Shaders/Private/ThinTranslucentCommon.ush` |
| TwoSidedFoliage BxDF | `Engine/Shaders/Private/ShadingModels.ush:844-955` |
| 体积光照采样 | `Engine/Shaders/Private/TranslucencyVolumeCommon.ush` |
| ShadingModel 定义 | `Engine/Classes/Engine/EngineTypes.h` |
