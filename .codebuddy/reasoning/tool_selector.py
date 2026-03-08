"""
Tool Selector - MCP 工具选择器

根据任务需求选择最合适的 MCP 工具组合。
"""

from dataclasses import dataclass
from typing import Dict, List, Optional, Set
from enum import Enum


class MCPType(Enum):
    """MCP 类型"""
    ANALYSIS = "analysis"       # 分析类
    CREATION = "creation"       # 创作类
    VALIDATION = "validation"   # 验证类


@dataclass
class MCPInfo:
    """MCP 信息"""
    name: str
    mcp_type: MCPType
    description: str
    capabilities: Set[str]      # 能力标签
    tools: List[str]            # 提供的工具列表


# MCP 注册表
MCP_REGISTRY: Dict[str, MCPInfo] = {
    "renderdoc": MCPInfo(
        name="renderdoc",
        mcp_type=MCPType.ANALYSIS,
        description="GPU 捕获分析 - 读取 RenderDoc 捕获数据",
        capabilities={
            "capture_analysis", "draw_call_inspection", "shader_analysis",
            "texture_extraction", "mesh_export", "pipeline_state",
            "gpu_timing", "buffer_read"
        },
        tools=[
            "get_capture_status", "open_capture", "list_captures", "get_frame_summary",
            "get_draw_calls", "get_draw_call_details", "find_draws_by_shader",
            "find_draws_by_texture", "find_draws_by_resource", "get_action_timings",
            "get_shader_info", "get_pipeline_state", "get_texture_info",
            "get_texture_data", "save_texture", "get_mesh_data", "export_mesh_as_fbx",
            "get_buffer_contents", "export_mesh_csv"
        ]
    ),
    
    "unreal-render": MCPInfo(
        name="unreal-render",
        mcp_type=MCPType.CREATION,
        description="UE 渲染资产创建 - 材质、网格、特效",
        capabilities={
            "material_creation", "material_graph", "texture_import",
            "mesh_import", "niagara_editing", "actor_spawning",
            "viewport_capture", "asset_management"
        },
        tools=[
            "build_material_graph", "compile_material", "get_material_graph",
            "get_material_function_content", "import_texture", "set_texture_properties",
            "import_fbx", "create_static_mesh_from_data", "get_viewport_screenshot",
            "spawn_actor", "delete_actor", "get_actors", "set_actor_properties",
            "get_actor_properties", "batch_spawn_actors", "batch_delete_actors",
            "batch_set_actors_properties", "create_asset", "delete_asset",
            "set_asset_properties", "get_asset_properties", "get_assets",
            "batch_create_assets", "batch_set_assets_properties",
            "get_niagara_asset_details", "update_niagara_asset",
            "analyze_stateless_compatibility", "convert_to_stateless",
            "get_niagara_module_graph", "get_niagara_script_asset",
            "update_niagara_script_asset"
        ]
    ),
    
    "lookdev": MCPInfo(
        name="lookdev",
        mcp_type=MCPType.VALIDATION,
        description="视觉验证环境 - 材质球、光照设置",
        capabilities={
            "visual_validation", "lighting_setup", "material_ball",
            "screenshot_comparison"
        },
        tools=[
            "setup_lookdev_env", "create_material_ball", "capture_validation_shot"
        ]
    ),
}


@dataclass
class ToolRecommendation:
    """工具推荐"""
    mcp_name: str
    tool_name: str
    reason: str
    priority: int  # 1=首选, 2=备选...


# 任务到能力的映射
TASK_CAPABILITY_MAP = {
    "analyze_shader": ["shader_analysis", "pipeline_state"],
    "extract_texture": ["texture_extraction"],
    "export_mesh": ["mesh_export"],
    "create_material": ["material_creation", "material_graph"],
    "import_asset": ["texture_import", "mesh_import"],
    "spawn_object": ["actor_spawning"],
    "edit_niagara": ["niagara_editing"],
    "capture_viewport": ["viewport_capture"],
    "analyze_draw_calls": ["draw_call_inspection", "capture_analysis"],
    "profile_gpu": ["gpu_timing"],
}


def get_required_capabilities(task: str) -> List[str]:
    """获取任务所需的能力"""
    return TASK_CAPABILITY_MAP.get(task, [])


def find_mcps_with_capability(capability: str) -> List[MCPInfo]:
    """找到具有指定能力的所有 MCP"""
    return [
        mcp for mcp in MCP_REGISTRY.values()
        if capability in mcp.capabilities
    ]


def select_tools_for_task(task: str, context: Optional[Dict] = None) -> List[ToolRecommendation]:
    """
    为任务选择最合适的工具
    
    Args:
        task: 任务类型
        context: 额外上下文（可选）
        
    Returns:
        工具推荐列表
    """
    capabilities = get_required_capabilities(task)
    if not capabilities:
        return []
    
    recommendations = []
    priority = 1
    
    for cap in capabilities:
        mcps = find_mcps_with_capability(cap)
        for mcp in mcps:
            # 找到与能力最相关的工具
            relevant_tools = _find_relevant_tools(mcp, cap, task)
            for tool in relevant_tools:
                recommendations.append(ToolRecommendation(
                    mcp_name=mcp.name,
                    tool_name=tool,
                    reason=f"MCP '{mcp.name}' 提供能力 '{cap}'",
                    priority=priority
                ))
        priority += 1
    
    return recommendations


def _find_relevant_tools(mcp: MCPInfo, capability: str, task: str) -> List[str]:
    """找到与能力最相关的工具"""
    # 简单实现：返回所有工具
    # TODO: 更智能的工具匹配
    return mcp.tools


def select_mcp_for_workflow(workflow: str) -> List[str]:
    """
    为工作流选择需要的 MCP 列表
    
    Args:
        workflow: 工作流名称
        
    Returns:
        MCP 名称列表
    """
    workflow_mcps = {
        "capture-to-asset": ["renderdoc", "unreal-render"],
        "shader-porting": ["renderdoc", "unreal-render"],
        "performance-audit": ["renderdoc"],
        "material-validation": ["unreal-render", "lookdev"],
        "lookdev-setup": ["unreal-render", "lookdev"],
    }
    return workflow_mcps.get(workflow, [])


def get_tool_description(mcp_name: str, tool_name: str) -> Optional[str]:
    """获取工具描述"""
    mcp = MCP_REGISTRY.get(mcp_name)
    if mcp and tool_name in mcp.tools:
        return f"Tool '{tool_name}' from MCP '{mcp_name}' ({mcp.description})"
    return None


def list_available_mcps() -> List[Dict]:
    """列出所有可用的 MCP"""
    return [
        {
            "name": mcp.name,
            "type": mcp.mcp_type.value,
            "description": mcp.description,
            "tool_count": len(mcp.tools),
            "capabilities": list(mcp.capabilities)
        }
        for mcp in MCP_REGISTRY.values()
    ]


def format_recommendations(recommendations: List[ToolRecommendation]) -> str:
    """格式化推荐结果"""
    lines = ["## 工具推荐", ""]
    
    for rec in recommendations:
        lines.append(f"- **{rec.mcp_name}**`{rec.tool_name}`")
        lines.append(f"  - 原因: {rec.reason}")
        lines.append(f"  - 优先级: {rec.priority}")
    
    return "\n".join(lines)
