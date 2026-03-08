# RenderDoc MCP Tools Reference

GPU capture analysis and asset extraction from RenderDoc.

## Overview

RenderDoc MCP 提供 GPU 捕获分析能力，支持：
- 帧分析与性能诊断
- Shader 逆向分析
- 纹理提取
- 网格导出

---

## Capture Tools (4 tools)

| Tool | Purpose |
|------|---------|
| `get_capture_status` | 检查 capture 是否加载 |
| `open_capture(capture_path)` | 打开 .rdc 文件 |
| `list_captures(directory)` | 列出目录下的 .rdc 文件 |
| `get_frame_summary` | 获取帧统计信息 |

### Examples

```python
# Check capture status
status = get_capture_status()
# Returns: {"loaded": true, "api": "D3D12"}

# Open capture
open_capture("E:/captures/scene.rdc")

# Get frame summary
summary = get_frame_summary()
# Returns: api, action_count, draw_calls, dispatches, resource_counts
```

---

## Draw Call Tools (6 tools)

| Tool | Purpose |
|------|---------|
| `get_draw_calls(marker_filter?, flags_filter?)` | 获取所有 draw calls |
| `get_draw_call_details(event_id)` | 获取 draw call 详情 |
| `find_draws_by_shader(shader_name, stage?)` | 按着色器查找 |
| `find_draws_by_texture(texture_name)` | 按纹理查找 |
| `find_draws_by_resource(resource_id)` | 按资源 ID 查找 |
| `get_action_timings(event_ids?)` | 获取 GPU 时间 |

### Examples

```python
# Get all draw calls
draws = get_draw_calls(flags_filter=["Drawcall"])

# Filter by marker
draws = get_draw_calls(marker_filter="Opaque")

# Find by shader
draws = find_draws_by_shader("BasePass", stage="pixel")

# Find by texture
draws = find_draws_by_texture("BaseColor")

# Get timing
timings = get_action_timings()
# Returns: timings, total_duration_ms
```

---

## Shader Tools (2 tools)

| Tool | Purpose |
|------|---------|
| `get_shader_info(event_id, stage)` | 获取着色器信息 |
| `get_pipeline_state(event_id)` | 获取完整管线状态 |

### get_shader_info

```python
info = get_shader_info(event_id=100, stage="pixel")
# Returns:
# - disassembly: DXBC/SPIR-V 反汇编
# - cbuffer_values: 常量缓冲区值
# - resources: 资源绑定 (textures, buffers, samplers)
```

### get_pipeline_state

```python
state = get_pipeline_state(event_id=100)
# Returns:
# - shaders: 各阶段着色器入口点
# - srvs: Shader Resource Views
# - uavs: Unordered Access Views
# - samplers: 采样器状态
# - render_targets: 渲染目标
# - viewports: 视口信息
```

---

## Texture Tools (3 tools)

| Tool | Purpose |
|------|---------|
| `get_texture_info(resource_id)` | 获取纹理元数据 |
| `get_texture_data(resource_id, mip?, slice?)` | 获取纹理像素数据 |
| `save_texture(resource_id, output_path, file_type?)` | 保存纹理到文件 |

### Examples

```python
# Get texture info
info = get_texture_info("ResourceId::12345")
# Returns: width, height, format, mip_levels, array_size

# Get texture data
data = get_texture_data("ResourceId::12345", mip=0)

# Save texture
save_texture(
    resource_id="ResourceId::12345",
    output_path="D:/output/texture.png",
    file_type="PNG"  # PNG, TGA, DDS, JPG, HDR, BMP, EXR
)
```

---

## Mesh Tools (3 tools)

| Tool | Purpose |
|------|---------|
| `get_mesh_data(event_id)` | 获取 mesh 数据 |
| `export_mesh_csv(event_id, output_path, stage?)` | 导出 CSV |
| `export_mesh_as_fbx(event_id, output_path, ...)` | 导出 FBX |

### get_mesh_data

```python
mesh = get_mesh_data(event_id=100)
# Returns:
# - vertex_positions
# - normals
# - uvs
# - tangents
# - indices
```

### export_mesh_as_fbx

```python
export_mesh_as_fbx(
    event_id=100,
    output_path="D:/output/mesh.fbx",
    attribute_mapping={
        "POSITION": "vs_input:ATTRIBUTE0",
        "NORMAL": "vs_output:TEXCOORD1",
        "UV": "vs_output:TEXCOORD0"
    },
    coordinate_system="ue",  # ue, unity, blender, maya
    unit_scale=1
)
```

---

## Buffer Tools (1 tool)

| Tool | Purpose |
|------|---------|
| `get_buffer_contents(resource_id, offset?, length?)` | 读取 buffer 数据 |

```python
data = get_buffer_contents(
    resource_id="ResourceId::12345",
    offset=0,
    length=1024
)
# Returns: base64-encoded data with metadata
```

---

## Typical Workflows

### 性能分析

```python
# 1. 获取帧概览
summary = get_frame_summary()

# 2. 获取时间分析
timings = get_action_timings()

# 3. 定位瓶颈 draw call
bottleneck = max(timings["timings"], key=lambda x: x["duration_ms"])

# 4. 分析 shader
shader_info = get_shader_info(bottleneck["event_id"], "pixel")
```

### 纹理提取

```python
# 1. 找到使用纹理的 draw call
draws = find_draws_by_texture("BaseColor")

# 2. 获取管线状态
state = get_pipeline_state(draws[0]["event_id"])

# 3. 找到纹理资源 ID
for srv in state["srvs"]:
    if "BaseColor" in srv["name"]:
        resource_id = srv["resource_id"]
        break

# 4. 保存纹理
save_texture(resource_id, "D:/output/BaseColor.png")
```

### 网格导出

```python
# 1. 获取 draw calls
draws = get_draw_calls(flags_filter=["Drawcall"])

# 2. 导出 CSV 分析顶点格式
export_mesh_csv(draws[0]["event_id"], "D:/output/mesh.csv")

# 3. 确定 attribute_mapping 后导出 FBX
export_mesh_as_fbx(
    event_id=draws[0]["event_id"],
    output_path="D:/output/mesh.fbx",
    attribute_mapping={
        "POSITION": "vs_input:ATTRIBUTE0",
        "NORMAL": "vs_output:TEXCOORD1",
        "UV": "vs_output:TEXCOORD0"
    }
)
```

---

## Notes

### renderdoc-mcp 连接方式

RenderDoc MCP 通过 TCP 连接到 RenderDoc 扩展：
1. 需要先运行 `python scripts/renderdoc/install_extension.py` 安装扩展
2. 启动 RenderDoc 并打开 capture
3. 扩展会自动启动 TCP 服务端

### 常见问题

| 问题 | 解决方案 |
|------|----------|
| "Method not found" | 确保扩展已安装且 RenderDoc 已重启 |
| 连接失败 | 检查 RenderDoc 是否已打开 capture |
| 纹理提取为黑色 | 可能是深度纹理或 HDR，尝试不同保存格式 |
