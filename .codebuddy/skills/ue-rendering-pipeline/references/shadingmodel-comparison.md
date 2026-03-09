# ShadingModel 对比分析：DefaultLit vs TwoSidedFoliage vs Subsurface

## 概述

本文档对比分析 UE5 中三种常用 ShadingModel 在不同材质类型和光照模式下的行为差异。

---

## 1. ShadingModel 核心差异

### 1.1 GBuffer 数据存储

| ShadingModel | CustomData.rgb | CustomData.a | DiffuseColor |
|--------------|---------------|--------------|--------------|
| **DefaultLit** | 未使用 | 未使用 | `BaseColor * (1 - Metallic)` |
| **TwoSidedFoliage** | SubsurfaceColor | Opacity | `BaseColor * (1 - Metallic)` |
| **Subsurface** | SubsurfaceColor | Opacity | `BaseColor * (1 - Metallic)` |

### 1.2 BxDF 光照计算

#### DefaultLit
```hlsl
FDirectLighting DefaultLitBxDF(...)
{
    // 前向 Diffuse
    Lighting.Diffuse = Diffuse_Lambert(DiffuseColor) * NoL;
    // 前向 Specular
    Lighting.Specular = D_GGX(...) * Vis * F_Schlick(...);
    // 无 Transmission
    Lighting.Transmission = 0;
}
```

#### TwoSidedFoliage
```hlsl
FDirectLighting TwoSidedBxDF(...)
{
    // 1. 前向光照：使用 BaseColor (DiffuseColor)
    FDirectLighting Lighting = DefaultLitBxDF(GBuffer, N, V, AreaLight, Shadow);
    
    // 2. 背光透射：使用 SubsurfaceColor
    half Wrap = 0.5;
    half WrapNoL = saturate((-dot(N, L) + Wrap) / Square(1 + Wrap));
    half VoL = dot(V, L);
    float Scatter = D_GGX(0.36, saturate(-VoL));
    
    // ⚠️ Opacity 未被使用！
    Lighting.Transmission = Falloff * WrapNoL * Scatter * SubsurfaceColor;
}
```

#### Subsurface
```hlsl
FDirectLighting SubsurfaceBxDF(...)
{
    // 1. 前向光照：使用 BaseColor
    FDirectLighting Lighting = DefaultLitBxDF(GBuffer, N, V, AreaLight, Shadow);
    
    // 2. 获取参数
    half3 SubsurfaceColor = ExtractSubsurfaceColor(GBuffer);
    half Opacity = GBuffer.CustomData.a;  // ✓ Opacity 被使用
    
    // 3. InScatter：视角与光源方向相反时的散射
    half InScatter = pow(saturate(dot(L, -V)), 12) * lerp(3, 0.1, Opacity);
    
    // 4. WrappedDiffuse
    half WrappedDiffuse = pow(saturate(NoL * 0.667 + 0.333), 1.5) * 1.667;
    half NormalContribution = lerp(1, WrappedDiffuse, Opacity);
    half BackScatter = GBuffer.GBufferAO * NormalContribution / (PI * 2);
    
    // 5. Beer-Lambert 吸收
    half3 TransmittedColor = TransmittanceToExtinction(SubsurfaceColor, ...);
    
    // 6. Transmission
    Lighting.Transmission = Falloff * lerp(BackScatter, 1, InScatter) 
                          * lerp(TransmittedColor, SubsurfaceColor, Thickness);
}
```

---

## 2. 材质类型对比

### 2.1 不透明材质 (Opaque/Masked)

| 特性 | DefaultLit | TwoSidedFoliage | Subsurface |
|------|-----------|-----------------|------------|
| **Transmission 计算** | ❌ 无 | ✅ 有 | ✅ 有 |
| **Opacity 影响** | 无 | 无（存储但未使用） | ✅ 影响 InScatter 强度 |
| **间接光照** | `BaseColor * Indirect` | `BaseColor * Indirect` | `(BaseColor + SubsurfaceColor) * Indirect` |

**间接光照差异（DiffuseIndirectComposite.usf:580-584）：**
```hlsl
if (GBuffer.ShadingModelID == SHADINGMODELID_SUBSURFACE)
{
    float3 SubsurfaceColor = ExtractSubsurfaceColor(GBuffer);
    DiffuseColor += SubsurfaceColor;  // Subsurface 会叠加！
}
IndirectLighting.Diffuse = DiffuseIndirectLighting * DiffuseColor;
```

### 2.2 半透明材质 (Translucent)

半透明材质的光照计算取决于 **Lighting Mode**。

---

## 3. 半透明光照模式对比

### 3.1 光照模式概述

| 模式 | 计算位置 | Transmission | Specular | 性能 |
|------|---------|--------------|----------|------|
| **Volumetric PerVertex NonDirectional** | 顶点 | ❌ | ❌ | ★★★★★ |
| **Volumetric PerVertex Directional** | 顶点 | ❌ | ❌ | ★★★★☆ |
| **Volumetric Directional** | 像素 | ❌ | ❌ | ★★★☆☆ |
| **Surface LightingVolume** | 像素 | ✅ | ❌ | ★★☆☆☆ |
| **Surface ForwardShading** | 像素 | ✅ | ✅ | ★☆☆☆☆ |

### 3.2 PerVertex 模式详细分析

#### Volumetric PerVertex Directional

**顶点着色器（BasePassVertexShader.usf:148-175）：**
```hlsl
#if TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_DIRECTIONAL
    // 使用顶点法线（不是材质法线！）
    float3 AmbientLightingVector = GetAmbientLightingVectorFromTranslucentLightingVolume(...);
    float3 DirectionalLightingVector = GetDirectionalLightingVectorFromTranslucentLightingVolume(...);
    
    Output.VertexDiffuseLighting = GetVolumeLightingDirectional(
        AmbientLightingVector, 
        DirectionalLightingVector, 
        VertexParameters.TangentToWorld[2],  // 顶点法线！
        DirectionalLightingIntensity
    ).rgb;
#endif
```

**像素着色器（BasePassPixelShader.usf:274-276）：**
```hlsl
#if TRANSLUCENCY_LIGHTING_VOLUMETRIC_PERVERTEX_DIRECTIONAL
    GetVolumeLightingDirectional(
        BasePassInterpolants.AmbientLightingVector, 
        BasePassInterpolants.DirectionalLightingVector, 
        MaterialParameters.WorldNormal,  // 材质法线
        GBuffer.DiffuseColor,            // ⚠️ 只用 DiffuseColor
        DirectionalLightingIntensity, 
        DiffuseLighting, 
        VolumeLighting
    );
#endif
```

**核心函数（TranslucencyVolumeCommon.ush:76-82）：**
```hlsl
void GetVolumeLightingDirectional(..., float3 DiffuseColor, ...)
{
    // 球谐光照计算
    FTwoBandSHVector DiffuseTransferSH = CalcDiffuseTransferSH(WorldNormal * DirectionalLightingIntensity, 1);
    VolumeLighting = max(0, DotSH(TranslucentLighting, DiffuseTransferSH));
    
    // 最终输出
    InterpolatedLighting += DiffuseColor * VolumeLighting.rgb;
    // ⚠️ 没有 Transmission 计算！
}
```

#### PerVertex 模式下的 ShadingModel 差异

| ShadingModel | DiffuseColor 来源 | Transmission | 最终颜色 |
|--------------|------------------|--------------|----------|
| **DefaultLit** | `BaseColor * (1-Metallic)` | ❌ 无 | `DiffuseColor * VolumeLighting` |
| **TwoSidedFoliage** | `BaseColor * (1-Metallic)` | ❌ 无 | `DiffuseColor * VolumeLighting` |
| **Subsurface** | `BaseColor * (1-Metallic)` | ❌ 无 | `DiffuseColor * VolumeLighting` |

**关键发现：PerVertex 模式下，三种 ShadingModel 表现完全相同！**

### 3.3 Surface LightingVolume 模式

**像素着色器（BasePassPixelShader.usf:282-286）：**
```hlsl
#if TRANSLUCENCY_LIGHTING_VOLUMETRIC_DIRECTIONAL || TRANSLUCENCY_LIGHTING_SURFACE_LIGHTINGVOLUME
    float4 AmbientLightingVector = GetAmbientLightingVectorFromTranslucentLightingVolume(...);
    float3 DirectionalLightingVector = GetDirectionalLightingVectorFromTranslucentLightingVolume(...);
    GetVolumeLightingDirectional(..., DiffuseLighting, VolumeLighting);
#endif
```

**后续光照计算（BasePassPixelShader.usf:1397-1410）：**
```hlsl
#if TRANSLUCENCY_LIGHTING_SURFACE_FORWARDSHADING || TRANSLUCENCY_LIGHTING_SURFACE_LIGHTINGVOLUME
    // 调用完整的光照计算
    FDeferredLightingSplit ForwardDirectLighting = GetForwardDirectLightingSplit(
        ..., GBuffer, ...
    );
    
    Color += ForwardDirectLighting.DiffuseLighting.rgb;   // 包含 Transmission
    Color += ForwardDirectLighting.SpecularLighting.rgb;
#endif
```

**GetForwardDirectLightingSplit（ForwardLightingCommon.ush:242）：**
```hlsl
// 调用完整 BxDF
FDeferredLightingSplit NewLighting = GetDynamicLightingSplit(
    TranslatedWorldPosition, -CameraVector, GBufferData, ...
);
// → AccumulateDynamicLighting → BxDF（TwoSidedBxDF / SubsurfaceBxDF）
// → 包含 Transmission 计算！
```

#### Surface 模式下的 ShadingModel 差异

| ShadingModel | Diffuse | Transmission | 最终颜色 |
|--------------|---------|--------------|----------|
| **DefaultLit** | `BaseColor * NoL` | ❌ 无 | Diffuse + Specular |
| **TwoSidedFoliage** | `BaseColor * NoL` | ✅ `SubsurfaceColor * WrapNoL * Scatter` | Diffuse + Specular + Transmission |
| **Subsurface** | `BaseColor * NoL` | ✅ `SubsurfaceColor * InScatter * Opacity` | Diffuse + Specular + Transmission |

### 3.4 Surface ForwardShading 模式

与 Surface LightingVolume 类似，但额外计算：
- 高质量 Specular
- 更多光源
- 阴影计算

---

## 4. 参数作用总结

### 4.1 BaseColor

| 场景 | 作用 |
|------|------|
| **所有 ShadingModel** | 前向 Diffuse 光照颜色 |
| **Subsurface 间接光照** | 与 SubsurfaceColor 叠加后参与计算 |

### 4.2 SubsurfaceColor

| ShadingModel | 直接光照 | 间接光照 | PerVertex 模式 |
|--------------|---------|---------|---------------|
| **DefaultLit** | 不使用 | 不使用 | 不使用 |
| **TwoSidedFoliage** | Transmission 颜色 | 不使用 | 不使用 |
| **Subsurface** | Transmission 颜色 | **叠加到 DiffuseColor** | 不使用 |

### 4.3 Opacity

| ShadingModel | 用途 | 影响 |
|--------------|------|------|
| **DefaultLit** | 无 | - |
| **TwoSidedFoliage** | 存储但未使用 | ⚠️ 无实际效果 |
| **Subsurface** | InScatter 强度 | `lerp(3, 0.1, Opacity)` - 高 Opacity = 弱散射 |

**Subsurface 的 Opacity 行为：**
```hlsl
// Opacity = 0: InScatter 系数 = 3（强散射）
// Opacity = 1: InScatter 系数 = 0.1（弱散射）
half InScatter = pow(saturate(dot(L, -V)), 12) * lerp(3, 0.1, Opacity);
```

---

## 5. 完整对比矩阵

### 5.1 不透明材质

| 特性 | DefaultLit | TwoSidedFoliage | Subsurface |
|------|-----------|-----------------|------------|
| 前向Diffuse | BaseColor | BaseColor | BaseColor |
| 前向 Specular | ✅ | ✅ | ✅ |
| 背光Transmission | ❌ | ✅ SubsurfaceColor | ✅ SubsurfaceColor + Opacity |
| 间接光照 | BaseColor | BaseColor | **BaseColor + SubsurfaceColor** |
| Opacity 影响 | - | 无 | ✅ 散射强度 |

### 5.2 半透明 PerVertex 模式

| 特性 | DefaultLit | TwoSidedFoliage | Subsurface |
|------|-----------|-----------------|------------|
| Diffuse | BaseColor | BaseColor | BaseColor |
| Transmission | ❌ | ❌ | ❌ |
| Specular | ❌ | ❌ | ❌ |
| Opacity 影响 | 无 | 无 | 无 |
| **表现差异** | **相同** | **相同** | **相同** |

### 5.3 半透明 Surface 模式

| 特性 | DefaultLit | TwoSidedFoliage | Subsurface |
|------|-----------|-----------------|------------|
| Diffuse | BaseColor | BaseColor | BaseColor |
| Transmission | ❌ | ✅ | ✅ |
| Transmission 颜色 | - | SubsurfaceColor | SubsurfaceColor |
| Opacity 影响 | - | 无 | ✅ 散射强度 |
| Specular | ✅ | ✅ | ✅ |

---

## 6. 使用建议

### 6.1 雾片 / 烟雾 Billboard

**需要背光透射效果：**
```
Shading Model: TwoSidedFoliage
Lighting Mode: Surface LightingVolume
BaseColor: 雾的基础颜色
Subsurface Color: 透射颜色（暖色 = 阳光穿透）
Opacity: 无影响，可不设置
```

**性能优先（大量粒子）：**
```
Shading Model: DefaultLit（与 TwoSidedFoliage 表现相同）
Lighting Mode: Volumetric PerVertex Directional
BaseColor: 雾的颜色
// 无背光透射效果
```

### 6.2 半透明物体（蜡、玉石）

```
Shading Model: Subsurface
Lighting Mode: Surface LightingVolume 或 Surface ForwardShading
BaseColor: 物体基础颜色
Subsurface Color: 内部散射颜色
Opacity: 控制散射强度（0 = 强散射，1 = 弱散射）
```

### 6.3 植被叶子

**高质量：**
```
Shading Model: TwoSidedFoliage
Lighting Mode: Surface LightingVolume
BaseColor: 叶子颜色
Subsurface Color: 透光颜色（浅绿/黄绿）
```

**性能优先：**
```
Shading Model: TwoSidedFoliage
Lighting Mode: Volumetric PerVertex Directional
// 注意：PerVertex 模式无背光透射效果
```

---

## 7. 常见问题

### Q1: 为什么 PerVertex 模式下 DefaultLit 和 TwoSidedFoliage 表现相同？

**A:** PerVertex 模式只计算 `DiffuseColor * VolumeLighting`，不调用 BxDF 函数，因此 Transmission 不会被计算。两者的 DiffuseColor 计算方式相同。

### Q2: TwoSidedFoliage 的 Opacity 为什么不影响表现？

**A:** 源码中 Opacity 被存储到 `GBuffer.CustomData.a`，但在 `TwoSidedBxDF` 中从未被读取。这是一个设计遗留问题。

### Q3: 如何在 PerVertex 模式下实现背光透射？

**A:** 无法实现。必须使用 Surface LightingVolume 或 Surface ForwardShading 模式。或者通过材质 Emissive 手动模拟。

### Q4: Subsurface 的间接光照为什么更亮？

**A:** Subsurface ShadingModel 会将 SubsurfaceColor 叠加到 DiffuseColor 上参与间接光照计算，导致整体更亮。

---

## 8. 源码位置

| 功能 | 文件路径 |
|------|----------|
| GBuffer 设置 | `ShadingModelsMaterial.ush:12-189` |
| DefaultLit BxDF | `ShadingModels.ush:198-288` |
| TwoSidedBxDF | `ShadingModels.ush:844-955` |
| SubsurfaceBxDF | `ShadingModels.ush:717-751` |
| PerVertex 光照 | `BasePassVertexShader.usf:148-175` |
| VolumeLighting 函数 | `TranslucencyVolumeCommon.ush:76-100` |
| 间接光照处理 | `DiffuseIndirectComposite.usf:580-584` |
| Forward 光照 | `ForwardLightingCommon.ush:137-270` |
