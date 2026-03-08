# Shader Porting 工作流

## 概述

跨引擎/平台 Shader 移植与重建。

## 触发条件

- "移植 Shader"
- "转换这个着色器"
- "Shader 兼容性问题"

## 工作流步骤

### Phase 1: Shader 分析

```yaml
step: analyze_shader
mcp: renderdoc
tools:
  - get_shader_info
  - get_pipeline_state
output:
  - shader_type: vs/ps/cs
  - input_semantics: 输入语义
  - constants: 常量缓冲区
  - texture_bindings: 纹理绑定
```

### Phase 2: 语义映射

```yaml
step: map_semantics
reasoning:
  - 识别引擎特定的语义名称
  - 映射到目标引擎的等价物
  - 处理坐标系差异
output:
  - semantic_map: 语义映射表
  - coordinate_transform: 坐标变换需求
```

### Phase 3: UE 材质重建

```yaml
step: rebuild_in_ue
mcp: unreal-render
tools:
  - build_material_graph
output:
  - material_asset: UE 材质路径
```

## 常见映射规则

| 源引擎 | 语义 | UE 等价 |
|--------|------|---------|
| Unity | _MainTex | BaseColor Texture |
| Unity | _NormalMap | Normal Texture |
| 自定义 | POSITION | World Position |
| 自定义 | TEXCOORD0 | UV0 |

## 知识依赖

- `agent/knowledge/rendering/shader_semantics.md`
- `agent/knowledge/rendering/coordinate_systems.md`
