# Performance Audit 工作流

## 概述

GPU 性能分析与优化建议生成。

## 触发条件

- "分析性能"
- "优化帧率"
- "性能瓶颈"

## 工作流步骤

### Phase 1: 数据收集

```yaml
step: collect_data
mcp: renderdoc
tools:
  - get_frame_summary
  - get_action_timings
  - get_draw_calls
output:
  - frame_stats: 帧统计
  - timing_data: GPU 时序
  - draw_call_stats: Draw Call 分布
```

### Phase 2: 瓶颈识别

```yaml
step: identify_bottlenecks
reasoning:
  - 分析 GPU 时序，找出耗时最高的操作
  - 检查 Draw Call 数量和批次
  - 识别 overdraw 和带宽问题
output:
  - bottlenecks: 瓶颈列表
  - severity: 严重程度排序
```

### Phase 3: 优化建议

```yaml
step: generate_recommendations
reasoning:
  - 基于瓶颈类型匹配优化策略
  - 考虑引擎特定优化技术
output:
  - recommendations: 优化建议列表
  - priority: 优先级排序
```

## 性能指标基准

| 指标 | 良好 | 警告 | 危险 |
|------|------|------|------|
| Draw Calls/帧 | <2000 | 2000-5000 | >5000 |
| 帧时间 (60fps) | <16ms | 16-33ms | >33ms |
| 三角面数 (场景) | <5M | 5M-10M | >10M |

## 知识依赖

- `agent/knowledge/performance/gpu_optimization.md`
- `agent/knowledge/performance/profiling_techniques.md`
