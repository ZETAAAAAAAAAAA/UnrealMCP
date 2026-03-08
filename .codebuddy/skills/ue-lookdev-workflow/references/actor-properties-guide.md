# Actor 属性设置指南

本指南说明如何通过 MCP 工具设置 Actor 属性。

## 基本语法

```python
set_actor_properties(
    name="ActorName",
    properties={
        "property_name": value
    }
)
```

## 支持的属性类型

### 1. Transform 变换属性

使用**数组格式** `[x, y, z]`：

```python
# 位置
set_actor_properties(name="Sun", properties={"location": [0, 0, 1000]})

# 旋转
set_actor_properties(name="Sun", properties={"rotation": [-45, 0, 0]})

# 缩放
set_actor_properties(name="Ground", properties={"scale": [10, 10, 1]})
```

### 2. 灯光属性

使用**简单属性名**（自动映射到组件）：

| 属性名 | 说明 | 示例值 |
|--------|------|--------|
| `intensity` | 强度 | 150000 (DirectionalLight), 8192 (SkyLight) |
| `color` | 颜色 | {"R": 255, "G": 253, "B": 245} |
| `temperature` | 色温 | 6500 |
| `cast_shadows` | 投射阴影 | true/false |
| `source_radius` | 光源半径 | 0.5 |
| `attenuation_radius` | 衰减半径 | 1000 |

```python
# 方向光
set_actor_properties(name="Sun", properties={
    "intensity": 150000,
    "temperature": 6500,
    "source_radius": 0.5
})

# 天光
set_actor_properties(name="Sky", properties={
    "intensity": 8192
})

# 点光源/聚光灯
set_actor_properties(name="PointLight", properties={
    "intensity": 10000,
    "attenuation_radius": 500,
    "cast_shadows": True
})
```

### 3. 布尔属性

```python
set_actor_properties(name="Actor", properties={
    "bHidden": False,
    "bCastShadow": True
})
```

### 4. 数值属性

```python
set_actor_properties(name="Actor", properties={
    "CustomTimeDilation": 1.0,
    "NetUpdateFrequency": 100
})
```

## 不支持的属性类型

以下属性**无法通过反射设置**，需要在 UE 编辑器中手动调整：

| 属性 | 原因 | 替代方案 |
|------|------|----------|
| `LightComponent.Intensity` | ObjectProperty 不支持 | 使用简单属性名 `intensity` |
| `StaticMeshComponent.StaticMesh` | ObjectProperty 不支持 | 在 spawn_actor 时设置 |
| `Material` | ObjectProperty 不支持 | 使用 build_material_graph |

## 完整 LookDev 场景配置示例

```python
# 1. 创建关卡
create_level("/Game/Levels/LookDev")

# 2. 创建方向光
spawn_actor("DirectionalLight", name="Sun", location=[0, 0, 1000])
set_actor_properties(name="Sun", properties={
    "intensity": 150000,  # 150000 lux (晴天)
    "rotation": [-45, 0, 0]
})

# 3. 创建天光
spawn_actor("SkyLight", name="Sky")
set_actor_properties(name="Sky", properties={
    "intensity": 8192  # 2^13 对应 EV13
})

# 4. 创建大气
spawn_actor("SkyAtmosphere", name="Atmosphere")

# 5. 创建地面
spawn_actor("StaticMeshActor", name="Ground")
set_actor_properties(name="Ground", properties={
    "scale": [10, 10, 1]
})

# 6. 创建灰球
spawn_actor("StaticMeshActor", name="GraySphere", location=[0, 0, 50])
set_actor_properties(name="GraySphere", properties={
    "scale": [1, 1, 1]
})

# 7. 设置相机
set_viewport_camera(
    location=[200, 150, 150],
    rotation=[-30, -135, 0],
    fov=50
)

# 8. 截图
get_viewport_screenshot("D:/output/lookdev.png")
```

## 故障排除

### 属性设置失败

检查：
1. 属性名是否正确（区分大小写）
2. 是否使用了支持的格式（数组 vs 对象）
3. 属性是否为 ObjectProperty（不支持）

### 查找可用属性

```python
get_actor_properties(name="ActorName")
```

返回所有可读取的属性列表。

## 最佳实践

1. **优先使用简单属性名**：`intensity` 而非 `LightComponent.Intensity`
2. **Transform 使用数组格式**：`[x, y, z]` 而非 `{"x": x, "y": y, "z": z}`
3. **批量设置**：一次调用设置多个属性
4. **检查返回值**：确认 `modified_count` 和 `failed_count`
