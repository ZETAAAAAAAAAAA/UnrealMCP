"""
Problem Decomposer - TA 问题分解器

将用户问题分解为可执行的子任务序列。
"""

from dataclasses import dataclass
from enum import Enum
from typing import List, Optional


class ProblemType(Enum):
    """问题类型分类"""
    RENDERING = "rendering"         # 渲染问题
    MATERIAL = "material"           # 材质问题
    PERFORMANCE = "performance"     # 性能问题
    ASSET_PIPELINE = "asset_pipeline"  # 资产管线
    SHADER = "shader"               # 着色器问题
    UNKNOWN = "unknown"             # 未知类型


class Priority(Enum):
    """任务优先级"""
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"


@dataclass
class SubTask:
    """子任务"""
    id: str
    description: str
    problem_type: ProblemType
    priority: Priority
    required_mcps: List[str]        # 需要的 MCP
    dependencies: List[str]         # 依赖的其他子任务 ID
    validation_criteria: str        # 验证标准


@dataclass
class Decomposition:
    """问题分解结果"""
    original_query: str
    problem_types: List[ProblemType]
    sub_tasks: List[SubTask]
    suggested_workflow: Optional[str]  # 建议的工作流模板


# 问题类型识别关键词
PROBLEM_KEYWORDS = {
    ProblemType.RENDERING: [
        "渲染", "render", "draw call", "绘制", "帧率", "frame",
        "光照", "lighting", "阴影", "shadow", "后处理", "post process"
    ],
    ProblemType.MATERIAL: [
        "材质", "material", "贴图", "texture", "PBR", "roughness",
        "metallic", "normal", "base color", "albedo"
    ],
    ProblemType.PERFORMANCE: [
        "性能", "performance", "优化", "optimize", "FPS", "卡顿",
        "GPU", "CPU", "内存", "memory", "瓶颈", "bottleneck"
    ],
    ProblemType.SHADER: [
        "shader", "着色器", "HLSL", "GLSL", "材质图", "material graph",
        "编译错误", "compile error"
    ],
    ProblemType.ASSET_PIPELINE: [
        "导入", "import", "导出", "export", "FBX", "管线", "pipeline",
        "自动化", "automation", "批量", "batch"
    ],
}


def classify_problem(query: str) -> List[ProblemType]:
    """根据查询内容识别问题类型"""
    query_lower = query.lower()
    detected_types = []
    
    for ptype, keywords in PROBLEM_KEYWORDS.items():
        if any(kw in query_lower for kw in keywords):
            detected_types.append(ptype)
    
    return detected_types if detected_types else [ProblemType.UNKNOWN]


def decompose(query: str) -> Decomposition:
    """
    分解问题为子任务
    
    Args:
        query: 用户原始查询
        
    Returns:
        Decomposition: 分解结果
    """
    problem_types = classify_problem(query)
    sub_tasks = []
    
    # 基于问题类型生成子任务
    task_counter = 0
    
    if ProblemType.RENDERING in problem_types:
        task_counter += 1
        sub_tasks.append(SubTask(
            id=f"render_analyze_{task_counter}",
            description="分析渲染状态和 Draw Calls",
            problem_type=ProblemType.RENDERING,
            priority=Priority.HIGH,
            required_mcps=["renderdoc"],
            dependencies=[],
            validation_criteria="成功获取渲染管线状态"
        ))
        
        task_counter += 1
        sub_tasks.append(SubTask(
            id=f"render_validate_{task_counter}",
            description="验证渲染结果",
            problem_type=ProblemType.RENDERING,
            priority=Priority.MEDIUM,
            required_mcps=["unreal-render", "lookdev"],
            dependencies=[f"render_analyze_{task_counter-1}"],
            validation_criteria="渲染结果与预期一致"
        ))
    
    if ProblemType.MATERIAL in problem_types:
        task_counter += 1
        sub_tasks.append(SubTask(
            id=f"material_analyze_{task_counter}",
            description="分析材质属性和纹理",
            problem_type=ProblemType.MATERIAL,
            priority=Priority.HIGH,
            required_mcps=["renderdoc"],
            dependencies=[],
            validation_criteria="成功提取材质参数"
        ))
        
        task_counter += 1
        sub_tasks.append(SubTask(
            id=f"material_reconstruct_{task_counter}",
            description="重建材质资产",
            problem_type=ProblemType.MATERIAL,
            priority=Priority.HIGH,
            required_mcps=["unreal-render"],
            dependencies=[f"material_analyze_{task_counter-1}"],
            validation_criteria="材质在 UE 中正常显示"
        ))
    
    if ProblemType.PERFORMANCE in problem_types:
        task_counter += 1
        sub_tasks.append(SubTask(
            id=f"perf_profile_{task_counter}",
            description="性能 Profiling",
            problem_type=ProblemType.PERFORMANCE,
            priority=Priority.HIGH,
            required_mcps=["renderdoc"],
            dependencies=[],
            validation_criteria="获取完整的性能数据"
        ))
        
        task_counter += 1
        sub_tasks.append(SubTask(
            id=f"perf_optimize_{task_counter}",
            description="提出优化方案",
            problem_type=ProblemType.PERFORMANCE,
            priority=Priority.MEDIUM,
            required_mcps=["unreal-render"],
            dependencies=[f"perf_profile_{task_counter-1}"],
            validation_criteria="优化方案可执行"
        ))
    
    if ProblemType.SHADER in problem_types:
        task_counter += 1
        sub_tasks.append(SubTask(
            id=f"shader_analyze_{task_counter}",
            description="分析着色器代码",
            problem_type=ProblemType.SHADER,
            priority=Priority.HIGH,
            required_mcps=["renderdoc"],
            dependencies=[],
            validation_criteria="成功获取 Shader 源码"
        ))
    
    # 确定建议的工作流
    suggested_workflow = _suggest_workflow(problem_types)
    
    return Decomposition(
        original_query=query,
        problem_types=problem_types,
        sub_tasks=sub_tasks,
        suggested_workflow=suggested_workflow
    )


def _suggest_workflow(problem_types: List[ProblemType]) -> Optional[str]:
    """根据问题类型建议工作流"""
    if ProblemType.MATERIAL in problem_types and ProblemType.RENDERING in problem_types:
        return "capture-to-asset"
    elif ProblemType.SHADER in problem_types:
        return "shader-porting"
    elif ProblemType.PERFORMANCE in problem_types:
        return "performance-audit"
    return None


def format_decomposition(decomp: Decomposition) -> str:
    """格式化分解结果为可读文本"""
    lines = [
        f"## 问题分析",
        f"",
        f"**原始查询**: {decomp.original_query}",
        f"",
        f"**问题类型**: {', '.join(t.value for t in decomp.problem_types)}",
        f"",
        f"## 子任务分解",
        f"",
    ]
    
    for task in decomp.sub_tasks:
        lines.append(f"### {task.id}")
        lines.append(f"- **描述**: {task.description}")
        lines.append(f"- **优先级**: {task.priority.value}")
        lines.append(f"- **所需 MCP**: {', '.join(task.required_mcps)}")
        lines.append(f"- **验证标准**: {task.validation_criteria}")
        if task.dependencies:
            lines.append(f"- **依赖**: {', '.join(task.dependencies)}")
        lines.append("")
    
    if decomp.suggested_workflow:
        lines.append(f"**建议工作流**: `{decomp.suggested_workflow}`")
    
    return "\n".join(lines)


# 供 Agent 调用的主入口
def analyze(query: str) -> str:
    """
    分析问题并返回分解结果
    
    Args:
        query: 用户问题
        
    Returns:
        格式化的分解结果
    """
    decomp = decompose(query)
    return format_decomposition(decomp)
