---
name: ue-material-workflow
description: |
  Unreal Engine 材质系统的分析与制作工作流。
  
  触发场景：
  - 用户想要创建或修改 UE 材质
  - 用户想要分析现有材质的结构
  - 用户想要生成材质变体
  - 用户想要理解材质函数的用法
---

# UE 材质分析与制作

## Overview

本技能覆盖 UE 材质系统的完整工作流：
- 材质图分析与理解
- 材质图构建与修改
- 材质实例管理
- 材质函数应用

## 核心工作流

### 决策树

```
用户目标?
│
├─ 查看现有材质 ──→ analyze-material
├─ 创建新材质 ──→ create-material
├─ 修改现有材质 ──→ modify-material
├─ 创建材质变体 ──→ create-material-instance
└─ 学习材质函数 ──→ explore-material-function
```

---

## 1. 材质分析

### 获取材质图结构

```
get_material_graph(material_name) → 获取节点和连接关系
```

### 分析输出

- **节点列表**: 每个节点的类型、位置、属性
- **连接关系**: 节点间的数据流
- **材质属性**: BlendMode, ShadingModel, TwoSided 等

### 理解材质逻辑

1. **从输出节点逆向追踪**
   - Base Color → 漫反射来源
   - Normal → 法线来源
   - Roughness/Metallic → PBR 参数
   - Emissive → 自发光

2. **识别关键模式**
   - 纹理采样 → 贴图使用
   - 数学运算 → 参数化逻辑
   - 材质函数 → 复用逻辑

---

## 2. 材质创建

### 构建材质图

使用 `build_material_graph` 一次性构建完整材质：

```python
build_material_graph(
    material_name="M_MyMaterial",
    nodes=[
        # 纹理采样节点
        {
            "type": "TextureSample",
            "name": "DiffuseMap",
            "properties": {
                "Texture": "/Game/Textures/T_Diffuse"
            },
            "position": [-400, 0]
        },
        # 参数节点
        {
            "type": "ScalarParameter",
            "name": "Roughness",
            "properties": {
                "DefaultValue": 0.5
            },
            "position": [-400, 200]
        },
        # 输出节点（自动存在）
    ],
    connections=[
        {
            "from_node": "DiffuseMap",
            "from_output": "RGB",
            "to_node": "MaterialGraphNode_0",  # 主节点
            "to_input": "Base Color"
        },
        {
            "from_node": "Roughness",
            "from_output": "Output",
            "to_node": "MaterialGraphNode_0",
            "to_input": "Roughness"
        }
    ],
    properties={
        "BlendMode": "BLEND_Opaque",
        "ShadingModel": "MSM_DefaultLit"
    },
    compile=True
)
```

### 常用节点类型

| 类型 | 用途 |
|------|------|
| `TextureSample` | 纹理采样 |
| `TextureSampleParameter2D` | 可参数化纹理 |
| `ScalarParameter` | 浮点参数 |
| `VectorParameter` | 颜色/向量参数 |
| `Constant` | 固定值 |
| `Multiply`, `Add`, `Subtract` | 数学运算 |
| `Lerp` | 线性插值 |
| `Fresnel` | 菲涅尔效果 |
| `WorldPosition` | 世界坐标 |
| `ViewSize` | 视图相关 |

### 连接目标

主材质节点的主要输入：

| 输入 | 类型 | 说明 |
|------|------|------|
| Base Color | float3 | 漫反射颜色 |
| Metallic | float | 金属度 |
| Roughness | float | 粗糙度 |
| Normal | float3 | 切线空间法线 |
| Emissive Color | float3 | 自发光 |
| Ambient Occlusion | float | 环境光遮蔽 |
| Opacity | float | 透明度 |
| Opacity Mask | float | 透明遮罩 |

---

## 3. 材质修改

### 工作流程

1. **获取现有结构**
   ```
   graph = get_material_graph(material_name)
   ```

2. **分析需要修改的部分**
   - 添加新节点
   - 修改连接关系
   - 更新属性值

3. **重新构建**
   ```
   build_material_graph(..., nodes=updated_nodes, connections=updated_connections)
   ```

4. **编译验证**
   ```
   compile_material(material_name)
   ```

### 常见修改模式

**添加新参数**
```python
nodes.append({
    "type": "ScalarParameter",
    "name": "NewParam",
    "properties": {"DefaultValue": 1.0},
    "position": [x, y]
})
```

**修改连接**
```python
connections.append({
    "from_node": "NewParam",
    "from_output": "Output",
    "to_node": "MaterialGraphNode_0",
    "to_input": "Roughness"
})
```

---

## 4. 材质实例

### 创建材质实例

```python
create_asset(
    asset_type="MaterialInstance",
    name="MI_MyMaterial_Instance",
    path="/Game/Materials/",
    properties={
        "Parent": "/Game/Materials/M_MyMaterial"
    }
)
```

### 设置参数

```python
set_asset_properties(
    asset_path="/Game/Materials/MI_MyMaterial_Instance",
    properties={
        "ScalarParameterValues": [
            {"ParameterName": "Roughness", "ParameterValue": 0.3},
            {"ParameterName": "Metallic", "ParameterValue": 0.8}
        ],
        "VectorParameterValues": [
            {"ParameterName": "BaseColor", "ParameterValue": [0.5, 0.3, 0.2]}
        ]
    }
)
```

### 批量创建变体

```python
variants = [
    {"name": "MI_Gold", "Roughness": 0.2, "Metallic": 1.0, "BaseColor": [1.0, 0.8, 0.3]},
    {"name": "MI_Silver", "Roughness": 0.3, "Metallic": 1.0, "BaseColor": [0.9, 0.9, 0.9]},
    {"name": "MI_Rust", "Roughness": 0.9, "Metallic": 0.0, "BaseColor": [0.4, 0.2, 0.1]}
]

batch_create_assets(
    items=[
        {
            "asset_type": "MaterialInstance",
            "name": v["name"],
            "path": "/Game/Materials/Variants/",
            "properties": {
                "Parent": "/Game/Materials/M_MyMaterial"
            }
        }
        for v in variants
    ]
)
```

---

## 5. 材质函数

### 获取材质函数内容

```python
get_material_function_content(function_path)
```

### 返回信息

- 输入参数列表
- 输出参数列表
- 内部节点结构
- 描述信息

### 常用内置材质函数

| 函数 | 用途 |
|------|------|
| `BreakOutFloat3` | 分解向量 |
| `CheapContrast` | 对比度调整 |
| `DepthFade` | 深度淡入 |
| `WorldAlignedTexture` | 三向投影 |
| `ObjectScale` | 物体缩放 |
| `CameraDistanceFade` | 距离淡入 |

---

## 验证流程

### 视口截图验证

```python
# 1. 设置场景
spawn_actor(actor_class="StaticMesh", properties={"StaticMesh": "/Game/Meshes/SM_Test"})
set_actor_properties(name="TestMesh", properties={"Material": material_path})

# 2. 截图
get_viewport_screenshot(output_path="D:/output/preview.png")

# 3. 对比分析
# 用户对比预期效果
```

---

## 最佳实践

### 材质组织

```
/Game/Materials/
├── Master/          # 主材质
├── Instances/       # 材质实例
├── Functions/       # 材质函数
└── Layers/          # 材质分层
```

### 命名规范

| 类型 | 前缀 |
|------|------|
| Material | M_ |
| Material Instance | MI_ |
| Material Function | MF_ |

### 性能考虑

1. **减少纹理采样** - 使用通道打包
2. **使用材质实例** - 避免重复编译
3. **控制指令数** - 复杂度 vs 效果
4. **LOD 材质** - 远距离简化

---

## 相关 Skill

- **renderdoc-reverse-engineering**: 从捕获中逆向分析材质结构
- **ue-niagara-workflow**: 如需创建粒子材质
