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
│                            TA Agent Core                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                   │
│  │   Persona    │  │  Knowledge   │  │    Skills    │                   │
│  │   TA 身份    │  │  领域知识     │  │  工作流 SOP  │                   │
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
TA Agent
|
├── .codebuddy/                   # Agent 核心
│   ├── agents/                   # Agent 定义
│   │   └── TA-Agent.md
│   │
│   ├── rules/                    # 项目规则 (始终加载)
│   │   └── mcp-development.mdc
│   │
│   ├── skills/                   # 技能模块 (按需加载)
│   │   ├── renderdoc-reverse-engineering/
│   │   ├── ue-material-workflow/
│   │   └── ue-niagara-workflow/
│   │
│   └── knowledge/                # 领域知识
│       ├── mcp-tools/            # MCP 工具详细文档
│       └── ue-api/               # UE API 差异笔记
│
├── mcps/                         # MCP Server
│   ├── renderdoc_mcp/            # RenderDoc 分析 MCP
│   └── unreal_render_mcp/        # UE 创作类 MCP
│
├── src/extension/                # RenderDoc 扩展
│
├── plugins/unreal/               # UE C++ 插件
│
└── config/                       # 配置模板
```

---

## MCP 概览

### RenderDoc MCP - 分析类

GPU 捕获分析与资产提取工具。

| 类别 | 工具 |
|------|------|
| **捕获** | `get_capture_status`, `open_capture`, `list_captures`, `get_frame_summary` |
| **Draw Call** | `get_draw_calls`, `get_draw_call_details`, `find_draws_by_*`, `get_action_timings` |
| **Shader** | `get_shader_info`, `get_pipeline_state` |
| **纹理** | `get_texture_info`, `get_texture_data`, `save_texture` |
| **网格** | `get_mesh_data`, `export_mesh_as_fbx`, `export_mesh_csv` |

### Unreal Render MCP - 创作类

UE 资产与场景操作工具。

| 类别 | 工具 |
|------|------|
| **通用资产** | `create_asset`, `delete_asset`, `get_assets`, `set_asset_properties`, `batch_*` |
| **通用 Actor** | `spawn_actor`, `delete_actor`, `get_actors`, `set_actor_properties`, `batch_*` |
| **材质图** | `build_material_graph`, `get_material_graph`, `compile_material` |
| **纹理** | `import_texture`, `set_texture_properties` |
| **网格** | `import_fbx`, `create_static_mesh_from_data` |
| **Niagara** | `get_niagara_asset_details`, `update_niagara_asset`, `convert_to_stateless`, ... |
| **视口** | `get_viewport_screenshot` |

> 详细文档见 `.codebuddy/knowledge/mcp-tools/`

---

## 初始化

### 环境要求

| 依赖 | 版本 |
|------|------|
| Python | 3.10+ |
| RenderDoc | 1.20+ |
| Unreal Engine | 5.3+ (推荐 5.7) |

### 首次设置

```bash
# 1. 安装 Python 依赖
pip install -e .

# 2. 初始化 RenderDoc 扩展
python src/scripts/renderdoc/install_extension.py

# 3. 打开 UE 项目（让引擎自动编译插件）
# plugins/unreal/UnrealMCP/RenderingMCP/RenderingMCP.uproject
```

### MCP 配置

**CodeBuddy / Claude Code** (`~/.codebuddy/mcp.json`):

```json
{
  "mcpServers": {
    "renderdoc": {
      "command": "python",
      "args": ["-m", "mcp_server.server"],
      "cwd": "D:/CodeBuddy/rendering-mcp/mcps/renderdoc_mcp"
    },
    "unreal-render": {
      "command": "python",
      "args": ["server.py"],
      "cwd": "D:/CodeBuddy/rendering-mcp/mcps/unreal_render_mcp"
    }
  }
}
```

### 开发提醒

| 修改了... | 需要操作 |
|-----------|----------|
| MCP 服务器代码 | `Ctrl+Shift+P` → `Reload Window` |
| UE 插件代码 | 重新编译 UE 项目 |
| RenderDoc 扩展 | 重启 RenderDoc |

---

## 许可证

MIT License
