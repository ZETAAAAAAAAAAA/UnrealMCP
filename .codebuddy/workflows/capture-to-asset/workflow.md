# Capture to Asset 工作流

## 概述

从 RenderDoc 帧捕获中提取并重建 UE 资产。

## 触发条件

- "重建材质"
- "从捕获创建资产"
- "还原这个模型"

## 工作流步骤

### Phase 1: 分析捕获

```yaml
step: analyze_capture
mcp: renderdoc
tools:
  - open_capture
  - get_frame_summary
  - get_draw_calls
output:
  - capture_info: 帧基本信息
  - draw_call_list: 目标 Draw Call 列表
```

### Phase 2: 提取数据

```yaml
step: extract_data
mcp: renderdoc
tools:
  - get_shader_info        # Shader 分析
  - get_pipeline_state     # 管线状态
  - get_texture_data       # 纹理提取
  - get_mesh_data          # 网格数据
output:
  - shader_info: Shader 参数和逻辑
  - textures: 纹理数据列表
  - mesh_data: 顶点/索引数据
```

### Phase 3: 重建资产

```yaml
step: reconstruct_assets
mcp: unreal-render
tools:
  - import_texture         # 导入纹理
  - create_static_mesh_from_data  # 创建网格
  - build_material_graph   # 构建材质
output:
  - texture_assets: UE 纹理路径列表
  - mesh_asset: UE 网格路径
  - material_asset: UE 材质路径
```

### Phase 4: 验证

```yaml
step: validate
mcp: unreal-render
tools:
  - spawn_actor            # 生成测试对象
  - get_viewport_screenshot  # 截图对比
output:
  - validation_result: 通过/失败
```

## 知识依赖

- `agent/knowledge/rendering/pbr_theory.md`
- `agent/knowledge/materials/texture_types.md`

## 示例执行

```python
# Agent 执行逻辑
result = analyze("从 RenderDoc 捕获重建这个角色的材质")

# 自动分解为子任务
for task in result.sub_tasks:
    if task.required_mcps.includes("renderdoc"):
        mcp_call("renderdoc", task.tool, task.params)
    elif task.required_mcps.includes("unreal-render"):
        mcp_call("unreal-render", task.tool, task.params)
```
