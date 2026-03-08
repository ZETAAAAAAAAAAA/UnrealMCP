"""
Reasoning Module - 推理框架

提供问题分解和工具选择能力。
"""

from .problem_decomposer import (
    decompose,
    analyze,
    ProblemType,
    Priority,
    SubTask,
    Decomposition,
    format_decomposition,
)

from .tool_selector import (
    select_tools_for_task,
    select_mcp_for_workflow,
    list_available_mcps,
    get_tool_description,
    ToolRecommendation,
    MCPInfo,
    MCPType,
)

__all__ = [
    # Problem Decomposer
    "decompose",
    "analyze",
    "ProblemType",
    "Priority",
    "SubTask",
    "Decomposition",
    "format_decomposition",
    # Tool Selector
    "select_tools_for_task",
    "select_mcp_for_workflow",
    "list_available_mcps",
    "get_tool_description",
    "ToolRecommendation",
    "MCPInfo",
    "MCPType",
]
