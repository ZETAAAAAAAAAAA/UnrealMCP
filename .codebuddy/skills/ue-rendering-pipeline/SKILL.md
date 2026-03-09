---
name: ue-rendering-pipeline
description: This skill should be used when analyzing Unreal Engine rendering pipeline source code, understanding rendering passes (BasePass, ShadowPass, Lighting), RHI commands, GPU resource management, or debugging rendering issues. Trigger when the user asks about rendering implementation, wants to understand how UE renders frames, or needs to modify rendering behavior.
---

# UE 渲染管线分析 Skill

## 概述

此 Skill 用于分析 UE5 渲染管线源码，理解渲染流程、资源管理和 RHI 命令。

---

## 核心类与源码位置

### 渲染管线核心类

| 类 | 源码路径 | 作用 |
|---|---------|------|
| `FSceneRenderer` | `Engine/Source/Runtime/Renderer/Private/SceneRendering.h` | 场景渲染管理，每帧创建 |
| `FDeferredShadingSceneRenderer` | `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.h` | 延迟渲染器实现 |
| `FRenderResource` | `Engine/Source/Runtime/RHI/Public/RenderResource.h` | 渲染资源基类（纹理、缓冲区） |
| `FRHICommandList` | `Engine/Source/Runtime/RHI/Public/RHICommandList.h` | RHI 命令列表，跨平台封装 |
| `FSceneView` | `Engine/Source/Runtime/Engine/Public/SceneView.h` | 视图信息（相机、投影） |
| `FSceneViewFamily` | `Engine/Source/Runtime/Engine/Public/SceneView.h` | 视图族，多个视图集合 |

### RHI 相关

| 类 | 源码路径 | 作用 |
|---|---------|------|
| `FRHICommandListImmediate` | `RHI/Public/RHICommandList.h` | 立即执行模式 |
| `FRHIResource` | `RHI/Public/RHI.h` | RHI 资源基类 |
| `IPooledRenderTarget` | `Renderer/Public/RenderTargetPool.h` | 渲染目标池 |

---

## 延迟渲染管线流程

### 主要阶段

```
FDeferredShadingSceneRenderer::Render()
│
├── InitViews()                    // 可见性剔除、阴影设置
│   ├── FSceneRenderer::GatherDynamicMeshElements()
│   └── FSceneRenderer::ComputeVisibility()
│
├── RenderBasePass()               // GBuffer 生成
│   ├── RenderOpaque()
│   └── WriteGBuffer()
│
├── RenderShadowDepths()           // 阴影深度
│   ├── RenderShadowDepthMaps()
│   └── RenderCubemapShadows()
│
├── RenderLighting()               // 光照计算
│   ├── RenderDeferredLighting()
│   └── RenderTranslucentLighting()
│
├── RenderFog()                    // 雾效
│
├── RenderTranslucency()           // 半透明物体
│
└── RenderPostProcessing()         // 后处理
    ├── RenderPostProcessMaterial()
    └── FinalOutput()
```

### 关键源码文件

| 文件 | 内容 |
|------|------|
| `DeferredShadingRenderer.cpp` | 延迟渲染主流程 |
| `BasePassRendering.cpp` | BasePass 实现 |
| `ShadowDepthRendering.cpp` | 阴影渲染 |
| `LightRendering.cpp` | 光照渲染 |
| `PostProcessMaterialRendering.cpp` | 后处理材质 |

---

## 分析方法

### 1. 定位渲染 Pass

```bash
# 搜索 BasePass 相关代码
grep -r "RenderBasePass" E:/UE/UE_5.7/Engine/Source/Runtime/Renderer/Private/

# 搜索 GBuffer 结构
grep -r "GBuffer" E:/UE/UE_5.7/Engine/Source/Runtime/Renderer/Private/
```

### 2. 理解 RHI 命令

```cpp
// RHI 命令列表示例
FRHICommandListImmediate& RHICmdList = FRHICommandListImmediate::Get();

// 设置渲染目标
RHICmdList.SetRenderTargets(RenderTargets);

// 绘制调用
RHICmdList.DrawIndexedPrimitive(...);
```

### 3. 调试渲染流程

**控制台命令:**
- `r.VisualizeTexture 0` - 可视化渲染目标
- `profilegpu` - GPU 性能分析
- `stat gpu` - GPU 统计信息
- `r.SetNearClipPlane 10` - 设置近裁面

**RenderDoc 分析:**
- 捕获帧后查看各 Pass 的输入输出
- 分析 GBuffer 内容
- 查看渲染状态

---

## 常见分析场景

### 场景 1: 分析自定义 GBuffer

1. 定位 GBuffer 定义: `FDeferredShadingSceneRenderer::CreateGBuffer()`
2. 查看 GBuffer 写入: `FBasePassPixelShaderParameters`
3. 查看 GBuffer 读取: `FDeferredLightingPS`

### 场景 2: 分析阴影渲染

1. 定位入口: `RenderShadowDepths()`
2. 查看阴影设置: `SetupShadowDepthPass()`
3. 分析阴影投影: `FProjectedShadowInfo`

### 场景 3: 分析后处理

1. 定位入口: `RenderPostProcessing()`
2. 查看后处理材质: `FPostProcessMaterialNode`
3. 分析混合: `FPostProcessContext`

---

## UE5 渲染模块依赖

```
Engine/Source/Runtime/Renderer/
├── Private/
│   ├── SceneRendering.cpp          # 场景渲染主逻辑
│   ├── DeferredShadingRenderer.cpp # 延迟渲染
│   ├── BasePassRendering.cpp       # BasePass
│   ├── ShadowDepthRendering.cpp    # 阴影
│   └── PostProcessMaterial.cpp     # 后处理
├── Public/
│   ├── SceneRendering.h
│   └── RenderResource.h
```

```
Engine/Source/Runtime/RHI/
├── Public/
│   ├── RHI.h                  # RHI 基础定义
│   ├── RHICommandList.h       # 命令列表
│   └── RenderResource.h       # 资源管理
└── Private/
    ├── D3D11/                  # DirectX 11 实现
    ├── D3D12/                  # DirectX 12 实现
    └── Vulkan/                 # Vulkan 实现
```

---

## 分析工作流

### 新功能分析

1. **定位入口** - 搜索相关关键词找到入口函数
2. **跟踪调用链** - 从入口函数向下跟踪调用
3. **理解数据流** - 分析输入输出参数
4. **RHI 层分析** - 查看最终 GPU 命令

### 问题调试

1. **复现问题** - 使用 RenderDoc 捕获问题帧
2. **定位 Pass** - 分析哪个渲染阶段出问题
3. **检查状态** - 验证渲染状态是否正确
4. **对比参考** - 与正常帧对比差异

---

## 注意事项

1. **版本差异** - UE5.7 与旧版本可能有 API 变化
2. **平台差异** - 不同图形 API (DX12/Vulkan/Metal) 实现不同
3. **性能考虑** - 理解性能瓶颈需要 GPU Profiler

---

## References 文档

专题分析文档位于 `references/` 目录：

| 文档 | 内容 |
|------|------|
| `translucent-lighting.md` | 半透明光照系统：Volume Lighting、Separate Translucency、Light Shaft、OIT、Lumen 集成 |

**加载方式：** 根据分析主题按需加载对应文档到上下文。

---

## 相关 Skill

- `ue-coding-workflow` - 编译、错误处理
- `renderdoc-reverse-engineering` - GPU 捕获分析
