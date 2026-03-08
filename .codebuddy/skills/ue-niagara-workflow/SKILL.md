---
name: ue-niagara-workflow
description: |
  Unreal Engine Niagara 粒子系统的分析与制作工作流。
  
  触发场景：
  - 用户想要创建或修改 Niagara 粒子效果
  - 用户想要分析现有 Niagara 系统的结构
  - 用户想要优化粒子性能
  - 用户想要将 Standard 模式转换为 Stateless
---

# UE Niagara 分析与制作

## Overview

本技能覆盖 UE Niagara 粒子系统的完整工作流：
- Niagara 系统分析
- 发射器与模块配置
- 参数调整与优化
- Stateless 转换

## 核心工作流

### 决策树

```
用户目标?
│
├─ 查看现有效果 ──→ analyze-niagara
├─ 创建新效果 ──→ create-niagara
├─ 修改现有效果 ──→ modify-niagara
├─ 性能优化 ──→ optimize-niagara
└─ Stateless 转换 ──→ convert-stateless
```

---

## 1. Niagara 系统分析

### 获取系统详情

```python
get_niagara_asset_details(
    asset_path="/Game/Effects/NS_MyEffect",
    detail_level="full"  # 或 "overview"
)
```

### 分析输出结构

**Overview 级别**
- 发射器列表
- 每个发射器的模块统计
- 使用的纹理/网格资源

**Full 级别**
- 每个模块的参数值
- 模块连接关系
- 脚本执行顺序

### 理解 Niagara 层级

```
Niagara System (NS_)
├── Niagara Emitter (NE_)
│   ├── Spawn Script
│   ├── Update Script
│   └── Render Script
└── Parameters
    ├── System Parameters
    ├── Emitter Parameters
    └── User Parameters
```

---

## 2. Niagara 创建

### 使用 update_niagara_asset

```python
update_niagara_asset(
    asset_path="/Game/Effects/NS_NewEffect",
    operations=[
        # 添加发射器
        {
            "operation": "add_emitter",
            "emitter_name": "SpawnRate",
            "emitter_type": "CPU"  # 或 "GPU"
        },
        # 添加模块
        {
            "operation": "add_module",
            "emitter": "SpawnRate",
            "script_type": "spawn",
            "module_name": "SpawnRate",
            "properties": {
                "SpawnRate": 100.0
            }
        },
        # 设置参数
        {
            "operation": "set_parameter",
            "parameter_name": "User.Lifetime",
            "parameter_type": "float",
            "value": 2.0
        }
    ]
)
```

### 常用模块

| 模块类型 | Spawn | Update | Render |
|----------|-------|--------|--------|
| **生成** | SpawnRate, SpawnBurst, SpawnPerUnit | - | - |
| **生命周期** | InitializeLifetime | - | - |
| **位置** | InitializePosition, AddLocation | AddVelocity, Drag | - |
| **颜色** | InitializeColor | Color, ColorFade | - |
| **大小** | InitializeSize | Scale, ScaleByLife | - |
| **渲染** | - | - | Sprite, Mesh, Ribbon, Light |

---

## 3. Niagara 修改

### 修改工作流

1. **获取当前结构**
   ```
   details = get_niagara_asset_details(asset_path, detail_level="full")
   ```

2. **定位修改目标**
   - 哪个发射器？
   - 哪个脚本（spawn/update/render）？
   - 哪个模块？

3. **执行修改**
   ```python
   update_niagara_asset(
       asset_path=asset_path,
       operations=[
           {
               "operation": "set_module_property",
               "emitter": "MainEmitter",
               "script_type": "spawn",
               "module": "SpawnRate",
               "property": "SpawnRate",
               "value": 200.0
           }
       ]
   )
   ```

### 常见修改场景

**调整发射率**
```python
{"operation": "set_module_property", "property": "SpawnRate", "value": 50.0}
```

**修改颜色**
```python
{"operation": "set_module_property", "property": "Color", "value": [1.0, 0.5, 0.0]}
```

**调整大小**
```python
{"operation": "set_module_property", "property": "SpriteSize", "value": 100.0}
```

---

## 4. 性能优化

### 分析兼容性

```python
analyze_stateless_compatibility(
    asset_path="/Game/Effects/NS_MyEffect",
    emitter="MainEmitter"
)
```

### 兼容性检查项

| 检查项 | 说明 |
|--------|------|
| 无动态参数 | 所有参数必须在编译时确定 |
| 无事件响应 | 不支持事件触发 |
| 无数据接口 | 不支持外部数据源 |
| 无循环逻辑 | 不支持 while/for 循环 |

### 转换为 Stateless

```python
convert_to_stateless(
    asset_path="/Game/Effects/NS_MyEffect",
    emitter="MainEmitter",
    force=False  # 强制转换即使有警告
)
```

### Stateless 优势

- 更低的 CPU 开销
- 更好的缓存利用率
- 适合大量粒子场景

---

## 5. 模块图分析

### 获取模块图结构

```python
get_niagara_module_graph(
    asset_path="/Game/Effects/NS_MyEffect",
    emitter="MainEmitter",
    script="spawn",  # 或 "update"
    save_to="D:/output/module_graph.json"
)
```

### 分析模块依赖

- 输入参数
- 输出参数
- 内部依赖关系

---

## 6. 独立脚本资产

### 获取 Niagara Script

```python
get_niagara_script_asset(
    script_path="/Game/Effects/Scripts/NMS_MyScript",
    save_to="D:/output/script.json"
)
```

### 更新脚本

```python
update_niagara_script_asset(
    script_path="/Game/Effects/Scripts/NMS_MyScript",
    operations=[
        {
            "operation": "set_input_default",
            "input_name": "Magnitude",
            "value": 10.0
        }
    ]
)
```

---

## 验证流程

### 场景中测试

```python
# 1. 放置 Niagara Actor
spawn_actor(
    actor_class="NiagaraActor",
    name="TestEffect",
    properties={
        "Asset": "/Game/Effects/NS_MyEffect",
        "Location": {"x": 0, "y": 0, "z": 100}
    }
)

# 2. 视口截图
get_viewport_screenshot(output_path="D:/output/effect_preview.png")

# 3. 性能统计
# 通过 frame profiling 检查开销
```

---

## 最佳实践

### 命名规范

| 类型 | 前缀 |
|------|------|
| Niagara System | NS_ |
| Niagara Emitter | NE_ |
| Niagara Script | NMS_ |

### 性能建议

1. **优先 GPU 粒子** - 大量粒子场景
2. **使用 Stateless** - 当不需要动态参数时
3. **控制发射率** - 根据视距调整
4. **合并渲染器** - 减少绘制调用

### 层级组织

```
/Game/Effects/
├── Systems/      # Niagara Systems
├── Emitters/     # Niagara Emitters
├── Scripts/      # 自定义脚本
└── Textures/     # 粒子贴图
```

---

## 常见问题排查

### Q: 粒子不可见？
A: 检查：
1. 是否有 Render 模块
2. 材质是否正确
3. 粒子大小是否为 0

### Q: 性能太差？
A: 检查：
1. 发射率是否过高
2. 是否可以使用 GPU 模式
3. 是否可以转换为 Stateless

### Q: 参数不生效？
A: 检查：
1. 参数命名空间是否正确
2. 参数类型是否匹配
3. 是否被其他模块覆盖

---

## 相关 Skill

- **renderdoc-reverse-engineering**: 从捕获中分析粒子效果
- **ue-material-workflow**: 创建粒子材质
