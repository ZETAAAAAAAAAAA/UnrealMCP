# TA Agent Workspace

**AI-Powered Technical Artist** - 一个具备专业 TA 能力的智能代理。

## 核心理念

TA Agent 不只是被动响应工具调用，而是具备：
- **TA 领域知识**（材质、渲染、性能、管线）
- **问题分解能力**（分析→诊断→方案→执行）
- **工具编排能力**（选择合适的 MCP 工具组合）
- **学习迭代能力**（从结果中学习）


## 架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            TA Agent Core                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                   │
│  │   Persona    │  │  Knowledge   │  │  Reasoning   │                   │
│  │   TA 身份    │  │  领域知识    │  │  决策引擎    │                   │
│  └──────────────┘  └──────────────┘  └──────────────┘                   │
└───────────────────────────────┬─────────────────────────────────────────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         ▼                      ▼                      ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Analysis MCP   │    │   Creation MCP  │    │  Validation MCP │
│  分析类工具      │    │   创作类工具     │    │   验证类工具     │
├─────────────────┤    ├─────────────────┤    ├─────────────────┤
│ • RenderDoc     │    │ • Unreal Engine │    │ • Lookdev       │
│ • PIX (未来)    │    │ • Unity (未来)  │    │ • Performance   │
│ • Nsight (未来) │    │ • Blender (未来)│    │ • Quality Check │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

---

## 项目结构

```
ta-agent-workspace/
│
├── .codebuddy/                   # Agent 核心 (身份、知识、技能、工作流)
│   ├── agents/                   # Agent 身份定义
│   │   └── TA Agent.md           # TA 角色
│   │
│   ├── rules/                    # 工作规则 (自动应用)
│   │   └── rendering-mcp-skills.mdc
│   │
│   ├── persona/                  # TA 身份详情
│   │   └── ta_persona.md
│   │
│   ├── knowledge/                # 领域知识库
│   │   ├── material-functions/   # 材质函数参考
│   │   └── ue-api/               # UE API 参考
│   │
│   ├── skills/                   # 技能模块（工作流 SOP）
│   │   ├── lookdev/              # Lookdev 环境
│   │   ├── renderdoc-fbx-export/ # Mesh 导出
│   │   ├── renderdoc-material-reconstruction/
│   │   └── shadertoy-conversion/ # Shadertoy 转换
│   │
│   ├── reasoning/                # 推理框架
│   │   ├── problem_decomposer.py # 问题分解器
│   │   └── tool_selector.py      # 工具选择器
│   │
│   └── workflows/                # 完整工作流定义
│       ├── capture-to-asset/     # 捕获→资产重建
│       ├── shader-porting/       # Shader 移植
│       └── performance-audit/    # 性能审计
│
├── mcps/                         # MCP 工具生态
│   ├── renderdoc_mcp/            # RenderDoc 分析 MCP
│   │   └── mcp_server/
│   │       ├── server.py         # FastMCP 工具定义
│   │       └── bridge/           # TCP 客户端
│   │
│   └── unreal_render_mcp/        # UE 创作类 MCP (模块化)
│       ├── server.py             # 入口
│       ├── connection.py         # 连接管理
│       ├── common.py             # 通用工具
│       └── tools/                # 工具模块
│           ├── material.py
│           ├── actor.py
│           ├── asset.py
│           └── ...
│
├── src/extension/                # RenderDoc 扩展
│   ├── renderdoc_facade.py       # API 门面
│   ├── socket_server.py          # TCP 服务端
│   └── services/                 # 服务分解
│
├── plugins/unreal/               # UE C++ 插件
│   └── UnrealMCP/                # TCP 服务端 + 反射调用
│
└── docs/                         # 文档
    ├── tools/                    # 工具文档
    ├── tutorials/                # 教程
    └── examples/                 # 示例
```

---

## MCP 工具参考

### RenderDoc MCP (20 Tools) - 分析类

| 类别 | 工具 |
|------|------|
| **捕获** | `get_capture_status`, `open_capture`, `list_captures`, `get_frame_summary` |
| **Draw Call** | `get_draw_calls`, `get_draw_call_details`, `find_draws_by_*`, `get_action_timings` |
| **Shader** | `get_shader_info`, `get_pipeline_state` |
| **纹理** | `get_texture_info`, `get_texture_data`, `save_texture` |
| **网格** | `get_mesh_data`, `export_mesh_as_fbx`, `export_mesh_csv` |

### Unreal Render MCP (26 Tools) - 创作类

| 类别 | 工具 |
|------|------|
| **通用资产** | `create_asset`, `delete_asset`, `get_assets`, `set_asset_properties`, batch_* |
| **通用 Actor** | `spawn_actor`, `delete_actor`, `get_actors`, `set_actor_properties`, batch_* |
| **材质图** | `build_material_graph`, `get_material_graph`, `compile_material` |
| **纹理** | `import_texture`, `set_texture_properties` |
| **网格** | `import_fbx`, `create_static_mesh_from_data` |
| **Niagara** | `get_niagara_asset_details`, `update_niagara_asset`, `convert_to_stateless`, ... |
| **视口** | `get_viewport_screenshot` |

---

## 安装

### 前置要求

- Python 3.10+
- RenderDoc 1.20+
- Unreal Engine 5.3+

### 设置

```bash
# 1. 克隆并安装
git clone https://github.com/your-repo/ta-agent-workspace.git
cd ta-agent-workspace
pip install -e .

# 2. 安装 RenderDoc 扩展
python src/scripts/install_extension.py

# 3. 打开 UE 项目
# plugins/unreal/UnrealMCP/RenderingMCP/RenderingMCP.uproject
```

### MCP 配置

**Claude Code / CodeBuddy** (`~/.codebuddy/mcp.json`):
```json
{
  "mcpServers": {
    "renderdoc": {
      "command": "python",
      "args": ["-m", "mcp_server.server"],
      "cwd": "/path/to/ta-agent-workspace/mcps/renderdoc_mcp"
    },
    "unreal-render": {
      "command": "python",
      "args": ["server.py"],
      "cwd": "/path/to/ta-agent-workspace/mcps/unreal_render_mcp"
    }
  }
}
```

---

## 使用示例

### Agent 工作方式

```
用户: "帮我从这个 RenderDoc 捕获重建材质"

Agent 思考:
1. [Persona] 作为 TA，需要先分析捕获
2. [Decompose] 识别为"材质重建"任务
3. [Tool Select] 
   - renderdoc → get_shader_info, get_texture_data
   - unreal-render → build_material_graph
4. [Execute] 按工作流执行
5. [Validate] 截图对比验证
```

### 具体工具调用

```python
# 材质重建
open_capture(capture_path="E:/captures/scene.rdc")
get_draw_calls(flags_filter=["Drawcall"])
export_mesh_as_fbx(event_id=5249, output_path="output/mesh.fbx", ...)

# UE 创建
build_material_graph(
    material_name="M_Reconstructed",
    nodes=[...],
    connections=[...]
)
```

---

## 工作流

| 工作流 | 描述 |
|--------|------|
| **capture-to-asset** | 从捕获重建资产 |
| **shader-porting** | Shader 移植 |
| **performance-audit** | 性能审计 |

---

## 许可证

MIT License
