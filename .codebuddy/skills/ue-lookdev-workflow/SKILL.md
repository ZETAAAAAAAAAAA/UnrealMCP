---
name: ue-lookdev-workflow
description: This skill should be used when setting up physically based lighting environments in Unreal Engine, including HDRI capture and processing, LookDev material calibration, and Virtual Production lighting matching. Trigger when the user needs physical lighting parameters, camera exposure settings, or guidance on matching real-world lighting to virtual scenes.
---

# UE LookDev Workflow

物理灯光环境与材质开发工作流，通过物理参数固定变量，建立可靠的 LookDev 流程。

## 核心理念

**物理精确是起点，艺术化是终点。**

## 快速参考

### 晴天标准参数

| 参数 | 数值 | 说明 |
|------|------|------|
| EV (曝光) | 13 | 阳光16法则 |
| 白平衡 | 6500K | 日光标准 |
| 太阳强度 | 150000 lux | 直射光 |
| 天光强度 | 8192 | 2^13 |
| 太阳角度 | 0.5° | 晴天张角 |

### UE 必要设置

```
Project Settings → Engine → Rendering:
  ☑ Apply Pre-Exposure before writing to the scene color
  ☑ Extend default luminance range in Auto Exposure settings
```

## 工作流程

### 流程 A: 创建准确虚拟材质

```
1. 拍摄 Ground Truth（含辅助道具）
2. 在 UE 中还原物理灯光
3. 放置对照材质（灰球、金属球）
4. 调整灯光匹配 Ground Truth
5. 在此灯光下创建/校准材质
```

### 流程 B: 匹配虚拟灯光到真实

```
1. 获取虚拟场景灯光参数
2. 测量真实环境照度
3. 使用物理参数设置真实灯光
4. 微调匹配视觉效果
```

### 流程 C: 匹配真实灯光到虚拟

```
1. 拍摄 HDRI + Ground Truth
2. 在 UE 中重建灯光环境
3. 放置对照道具对比
4. 迭代调整至匹配
```

## LookDev 辅助工具

### UE 对照材质

```
引擎内置: /Engine/EditorMeshes/ColorCalibrator/SM_ColorCalibrator

自定义材质:
  灰球: BaseColor=0.18, Roughness=0.95, Metallic=0
  金属球: BaseColor=0.95, Roughness=0.1, Metallic=1
```

### 标准灰值

| 名称 | 线性值 | sRGB值 |
|------|--------|--------|
| 中灰 (18%) | 0.18 | 117 |
| 暗灰 | 0.10 | 77 |
| 亮灰 | 0.50 | 188 |

## 灯光类型与单位

| 灯光类型 | 强度单位 | 晴天物理参数 |
|----------|----------|--------------|
| DirectionalLight | lux | 150000 |
| PointLight | lm / cd | 参考 IES |
| SpotLight | lm / cd | 参考 IES |
| RectLight | lm / cd | 参考 IES |
| SkyLight | HDRI | 亮度来自 HDRI |

## 光质控制

| 属性 | 参数 | 效果 |
|------|------|------|
| Source Radius | 光源尺寸 | 越大越柔和 |
| Soft Source Radius | 离散度 | 阴影边缘柔和度 |
| Source Angle | 方向光张角 | 0.5° = 晴天 |

## 基础可照亮场景配置

UE5 默认可照亮场景包含以下必要 Actor：

### 必需 Actor

| Actor | 类型 | 位置 | 作用 |
|-------|------|------|------|
| DirectionalLight | 方向光 | (0, 0, 400) | 主光源（太阳/月亮） |
| SkyLight | 天光 | (0, 0, 600) | 环境反射光 |
| SkyAtmosphere | 大气 | (0, 0, -6000) | 天空盒、大气散射 |
| ExponentialHeightFog | 指数高度雾 | (-5600, -50, -6850) | 距离雾效 |
| VolumetricCloud | 体积云 | (0, 0, 700) | 云层效果 |
| StaticMeshActor | 地面 | (0, 0, 0) | 接收阴影的基础地面 |

### MCP 创建脚本

```python
# 1. 创建基础场景
create_level("/Game/Levels/LookDev")

# 2. 添加方向光（主光源）
spawn_actor("DirectionalLight", location={"x": 0, "y": 0, "z": 400})

# 3. 添加天光（环境光）
spawn_actor("SkyLight", location={"x": 0, "y": 0, "z": 600})

# 4. 添加大气效果
spawn_actor("SkyAtmosphere", location={"x": 0, "y": 0, "z": -6000})

# 5. 添加雾效
spawn_actor("ExponentialHeightFog", location={"x": -5600, "y": -50, "z": -6850})

# 6. 添加地面
spawn_actor("StaticMeshActor", location={"x": 0, "y": 0, "z": 0}, 
            properties={"StaticMeshComponent.StaticMesh": "/Engine/BasicShapes/Plane"})

# 7. 放置灰球
spawn_actor("StaticMeshActor", name="GraySphere", 
            location={"x": 0, "y": 0, "z": 50},
            properties={"StaticMeshComponent.StaticMesh": "/Engine/BasicShapes/Sphere"})
```

### 注意事项

- **DirectionalLight**: 必须添加，否则场景无直射光照
- **SkyLight**: 必须添加，否则无环境反射
- **SkyAtmosphere**: 提供天空背景和大气散射，对 LookDev 很重要
- **地面**: 必须有接收阴影的表面才能看到光照效果

## 常见问题

### Q: EV 值是否必须与拍摄同步？

可以不同步，但同步的好处：
- 太阳可直接用 150000 lux
- 人造光可直接用 IES 参数
- 光比关系准确

如果使用 EV=0，所有光强需除以 2^13。

### Q: EV 超过 15 怎么办？

开启 Pre-Exposure 可改善精度问题。

### Q: 最终画面对比太强？

物理灯光是起点，可在此基础上：
- 整体缩放光强
- 增加补光平衡光比
- 调整后期 Tonemapping

## Resources

### references/

详细参考文档，按需加载：

- `physical-lighting-parameters.md` - 完整物理灯光参数表、EV 对应关系
- `hdri-capture-guide.md` - HDRI 拍摄与处理完整流程
- `lookdev-tools.md` - 辅助工具使用、材质对照表
- `actor-properties-guide.md` - MCP Actor 属性设置指南
