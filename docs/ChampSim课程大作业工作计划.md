# ChampSim 课程大作业工作计划
## 结构化指标、批量实验、自动比较与预取研究扩展

> 项目主线：在 ChampSim 现有仿真核心之上，构建一套面向微架构研究的实验平台。第一阶段优先完成结构化指标输出和多预取器/多 Trace 的批量运行，第二阶段完成自动比较、图表报告，并在仿真侧实现一个新增预取研究指标。

---

## 1. 参考作业分析与对本项目的启示

### 1.1 参考材料

参考作业位于 `ucas-oop-master/Project`，主要包含：

```text
docs/report.pdf       最终版报告
docs/report0.pdf      第 7-8 周系统分析与设计报告
docs/report1.pdf      第二版扩展报告
docs/slide0.pdf       第一次课堂汇报
docs/slide1.pdf       第二次课堂汇报
docs/raw/             报告、讲稿、设计模式材料和代码说明
Project/              实际代码、测试、示例和文档
```

### 1.2 参考报告的优点

1. 初版报告先讲系统背景、模块划分、核心流程、扩展点、测试与风险，再进入实现。
2. 最终报告从缺陷和需求出发，而不是直接罗列代码。
3. 明确使用建造者模式和策略模式解释重构方案，并说明为什么需要这些抽象。
4. 每个扩展点都有输入、输出、调用流程、异常和验收标准。
5. 给出了测试覆盖、代码规模、性能数据和效果截图。
6. 报告最后公开说明大模型辅助范围和参考资料，过程较透明。

### 1.3 需要吸取的风险

参考仓库中部分功能存在“报告阶段完整、代码仍是框架或占位”的情况，例如 `pipeline_wan_fun_control.py` 中仍可看到 TODO 和 `NotImplementedError`。同时，参考材料跨越了多个扩展方案，报告主线相对分散。

本项目必须坚持：

- 最终提交中的每个核心功能都必须有可运行路径。
- 已有功能和计划功能必须明确分开。
- 不由报告代替代码，不把接口骨架写成已经完成。
- 一个阶段只交付一个可验证闭环。
- 大模型生成的分析、代码和图表必须经过运行验证。
- 所有性能数字必须能由脚本和原始结果复现。

### 1.4 本项目的差异化重点

```text
参考作业：重构 + 算法扩展 + 测试与性能展示
本项目：ChampSim 仿真侧指标扩展 + 实验编排 + 自动比较 + 可视化 + 预取研究指标
```

本项目更强调：

- 仿真结果从固定文本输出升级为结构化数据模型；
- 多预取器和多 Trace 的公平批量实验；
- 实验数据能够自动比较和回溯；
- 新增指标能够支持真实的预取研究结论；
- 面向对象设计服务于研究流程，而不是只为展示设计模式。

---

## 2. 项目目标与范围

## 2.1 总目标

构建一个可运行的 ChampSim 实验平台，完成以下闭环：

```text
实验配置
  -> 多预取器/多 Trace 任务展开
  -> 有界并行运行
  -> 原始结果归档
  -> 结构化解析
  -> 指标计算
  -> 自动比较
  -> 图表与报告
  -> 结果追溯与复现
```

## 2.2 五个必须交付的模块

### 模块 A：结构化指标输出

- 将 ChampSim 的核心统计统一输出为 JSON/JSONL。
- 保留现有文本输出，避免破坏原功能。
- 为每次运行添加 `run_id`、阶段、缓存层级和作用域。
- 支持现有指标和新增指标的统一表达。

### 模块 B：多预取器/多 Trace 并行运行

- 支持一个实验定义多个预取器、多条 Trace 和多次重复。
- 采用“一任务一进程”的有界并行。
- 每个任务使用独立工作目录。
- 记录命令、配置、退出码、耗时和日志。
- 支持超时、失败重试和取消。

### 模块 C：自动比较

- 选择基线方案。
- 计算绝对变化、相对变化和加速比。
- 计算均值、中位数、标准差和置信区间。
- 输出结构化 `ComparisonResult`。
- 支持按预取器、Trace、缓存层级和指标分组。

### 模块 D：图表与报告

- 预取器对比柱状图。
- Trace 对比折线图。
- 预取器-Trace 热力图。
- 多次运行箱线图或误差条图。
- 自动生成 HTML/Markdown 报告。
- 图表和报告必须能追溯到 `run_id`、Trace、配置和原始结果。

### 模块 E：一个新增预取研究指标

首选新增指标：**预取提前量及其分布**。

```text
Prefetch Lead Time
= first demand use time - prefetch fill time
```

目标输出：

- `average_prefetch_lead_time`
- `median_prefetch_lead_time`
- `p90_prefetch_lead_time`
- `timely_prefetch_rate`
- `late_prefetch_rate`

该指标用于回答“预取是否足够早”，不能简单用现有的 `useful / issued` 解释。

---

## 3. 明确不做或暂缓的内容

第一版不做：

- 图形化桌面应用；
- 分布式集群调度；
- 自动机器学习调优；
- 完整 Cache Coherence；
- 完整功耗/温度模型；
- 运行时动态加载任意 C++ 模块；
- 同时运行多个预取器进行不公平的单方案性能比较；
- 自动生成研究结论或论文。

多个预取器若在同一个缓存中同时运行，只能作为“预取器交互实验”，不能与分别独立运行的结果混合比较。

### 3.1 构建环境状态

Docker 构建环境已经建立并验证：

```text
镜像：champsim-dev:ubuntu22.04
Ubuntu：22.04.5 LTS
GCC：11.4.0
CMake：3.22.1
GNU Make：4.3
Python：3.10.12
vcpkg：2026-09-26
```

容器入口：

```powershell
docker compose run --rm champsim
```

已完成验证：

- `python3 config.sh champsim_config.json` 通过；
- `make -j$(nproc)` 通过；
- `make -j$(nproc) test` 通过；
- C++ 测试 11388 个断言、624 个测试用例全部通过；
- `make pytest` 通过；
- Python 测试 228 个通过、1 个跳过；
- `bin/champsim --help` 通过。

后续 C++ 统计埋点、编译和测试统一在容器内执行，避免 Windows shell 与 Makefile 的兼容问题。

已完成 `429.mcf-217B.champsimtrace.xz` 快速基线：

```text
Warmup：1000000 instructions
Simulation：2000000 instructions
ROI instructions：2000004
ROI cycles：2642543
IPC：0.756848
Wall time：15.394851859 seconds
```

---

## 4. 总体系统架构

```text
+----------------------------------------------------------+
| ChampSim 仿真核心                                         |
| O3_CPU / CACHE / PTW / DRAM / 现有策略模块                |
+----------------------------------------------------------+
                  |
                  | MetricSink / JSONL
                  v
+----------------------------------------------------------+
| 仿真侧结构化指标层                                        |
| MetricRecord / MetricDefinition / MetricSink              |
| 自定义预取提前量采集                                       |
+----------------------------------------------------------+
                  |
                  | per-run result.json / metrics.jsonl
                  v
+----------------------------------------------------------+
| 实验编排与结果层                                          |
| ExperimentSpec / JobPlanner / ParallelRunScheduler         |
| RunExecutor / RunArtifact / ResultParser / RunResult        |
+----------------------------------------------------------+
                  |
                  v
+----------------------------------------------------------+
| 指标、比较与可视化层                                      |
| MetricRegistry / Aggregator / Comparator                   |
| ChartSpec / Renderer / ReportBuilder                       |
+----------------------------------------------------------+
```

依赖方向必须保持：

```text
仿真核心 -> 指标输出
实验编排 -> 运行结果
结果解析 -> 领域结果对象
比较和渲染 -> 结果对象
```

反向依赖禁止出现：

- Renderer 不启动 ChampSim；
- Comparator 不解析原始 stdout；
- Runner 不计算图表；
- CACHE 不知道图表对象；
- JSON Parser 不依赖实验调度器。

---

## 5. 对象模型与职责

### 5.1 ChampSim 仿真侧

| 对象 | 职责 |
| --- | --- |
| `MetricDefinition` | 指标 ID、单位、公式、作用域和版本 |
| `MetricRecord` | 一条结构化指标记录 |
| `MetricSink` | 接收并输出指标记录 |
| `JsonlMetricSink` | 以 JSONL 形式持久化指标 |
| `PrefetchTimelinessCollector` | 采集预取 issue、fill 和 demand use 时间 |
| `MetricSchemaVersion` | 管理指标格式版本 |

### 5.2 实验定义与执行层

| 对象 | 职责 |
| --- | --- |
| `ExperimentSpec` | 实验矩阵和基线配置 |
| `RunSpec` | 单个运行任务 |
| `JobPlanner` | 展开预取器、Trace 和重复次数 |
| `ResourcePolicy` | 并行度、超时和资源限制 |
| `ParallelRunScheduler` | 排队、调度、取消和重试 |
| `RunExecutor` | 启动并监控独立 ChampSim 进程 |
| `RunWorkspace` | 管理单次运行目录 |
| `RunArtifact` | 原始日志、JSON、命令和状态 |

### 5.3 解析与分析层

| 对象 | 职责 |
| --- | --- |
| `ResultParser` | 将原始结果转为 `RunResult` |
| `JsonResultParser` | 解析 ChampSim JSON |
| `RunResult` | 一次运行的结构化结果 |
| `ResultSet` | 多次运行结果集合 |
| `MetricRegistry` | 注册和查找指标 |
| `MetricCalculator` | 计算派生指标 |
| `Aggregator` | 聚合多次重复运行 |
| `BaselineSelector` | 选择基线 |
| `Comparator` | 计算差异和加速比 |
| `ComparisonResult` | 结构化比较结果 |

### 5.4 可视化与报告层

| 对象 | 职责 |
| --- | --- |
| `ChartSpec` | 描述图表语义 |
| `Renderer` | 渲染器接口 |
| `BarChartRenderer` | 预取器对比图 |
| `LineChartRenderer` | Trace 或参数趋势图 |
| `HeatmapRenderer` | 预取器-Trace 热力图 |
| `ReportBuilder` | 组织表格、图表和说明 |
| `ArtifactStore` | 保存最终输出 |

---

## 6. 数据契约

### 6.1 实验配置示例

```json
{
  "experiment_id": "prefetch-study-001",
  "baseline": "no",
  "prefetchers": ["no", "next_line", "ip_stride", "spp_dev"],
  "traces": ["trace-a", "trace-b", "trace-c", "trace-d", "trace-e"],
  "repetitions": 3,
  "warmup_instructions": 200000000,
  "simulation_instructions": 500000000,
  "parallelism": 4
}
```

### 6.2 单次运行结果示例

```json
{
  "run_id": "run-0001",
  "experiment_id": "prefetch-study-001",
  "trace": "trace-a",
  "prefetcher": "ip_stride",
  "repeat": 1,
  "status": "succeeded",
  "phases": {
    "roi": {
      "ipc": 1.24,
      "l1d_mpki": 12.4,
      "prefetch_accuracy": 0.62,
      "prefetch_coverage": 0.38,
      "prefetch_mean_lead_time": 42.0
    }
  }
}
```

### 6.3 指标记录示例

```json
{
  "schema_version": "1.0",
  "run_id": "run-0001",
  "metric_id": "prefetch_mean_lead_time",
  "scope": "cpu0->L2C",
  "phase": "roi",
  "value": 42.0,
  "unit": "core_cycles"
}
```

### 6.4 比较结果示例

```json
{
  "baseline": "no",
  "variant": "ip_stride",
  "trace": "trace-a",
  "metric": "ipc",
  "baseline_mean": 1.10,
  "variant_mean": 1.24,
  "absolute_delta": 0.14,
  "relative_delta": 0.127,
  "samples": 3
}
```

---

## 7. 新增预取指标设计

### 7.1 研究问题

现有统计可以回答：

```text
预取发出了多少？
有多少被使用？
有多少没有使用？
```

但不能直接回答：

```text
预取比 demand 提前了多少周期？
有多少预取来得足够早？
预取过晚是否限制了收益？
```

### 7.2 定义

对一次成功的预取：

```text
prefetch_issue_time
prefetch_fill_time
first_demand_use_time

lead_time
= first_demand_use_time - prefetch_fill_time
```

如果 demand 到达时间早于 fill 完成时间，则该预取属于 late prefetch，需要单独记录。

### 7.3 输出指标

```text
mean_lead_time
median_lead_time
p90_lead_time
timely_prefetch_rate
late_prefetch_rate
prefetch_lateness_cycles
```

### 7.4 采集位置

建议只在选择的研究层级启用，避免对所有缓存造成额外开销：

| 事件 | 建议位置 | 记录内容 |
| --- | --- | --- |
| 预取请求发出 | `CACHE::prefetch_line()` 路径 | 地址、缓存名、issue time |
| 预取填充完成 | `CACHE::handle_fill()` 路径 | fill time、Set、Way |
| Demand 使用预取块 | `CACHE::try_hit()` 或命中路径 | first use time、instr ID |

### 7.5 数据生命周期

- 只为启用计数的缓存建立有界记录表。
- 记录项在第一次 Demand 使用后移除。
- 被 Eviction 或失效的预取项应记录为未使用或过期。
- 不允许记录表无限增长。
- 指标关闭时，不应产生额外查找和内存增长。
- 指标开启和关闭时，被模拟的指令数、IPC 和缓存统计必须保持一致。

### 7.6 正确性验证

使用构造场景验证：

1. 预取在 Demand 前完成：lead time 为正。
2. Demand 与预取同时发生：lead time 为 0，按规则归类。
3. Demand 早于填充完成：记录为 late。
4. 预取被驱逐且从未使用：不计入 lead time，计入 unused/expired。
5. 多次访问只记录第一次 Demand 使用。
6. 多预取器启用时，记录来源模块标识。

---

## 8. 实验计划

### 8.1 实验对象

建议第一版选择 3-5 个预取器：

```text
no
next_line
ip_stride
spp_dev
可选：va_ampm_lite
```

### 8.2 Trace

至少 3 条，建议 5 条，覆盖：

- 计算密集；
- 分支密集；
- L1/L2 访存密集；
- 预取敏感；
- DRAM 带宽受限。

### 8.3 实验矩阵

```text
预取器数量：4
Trace 数量：5
重复次数：3
总任务数：60
并行度：1、4、8
```

时间不足时缩减为：

```text
3 个预取器 × 3 条 Trace × 3 次重复 = 27 个任务
```

### 8.4 主要指标

```text
IPC
L1D/L2C/LLC MPKI
Prefetch Accuracy
Prefetch Coverage
Average Miss Latency
Prefetch Mean Lead Time
Timely Prefetch Rate
DRAM Row Buffer Hit Rate
Data Bus Utilization
```

### 8.5 平台指标

```text
批量实验总 wall-clock 时间
不同并行度下的吞吐率
每个任务的运行时间
峰值内存占用
失败运行数量
重试次数
```

### 8.6 正确性门禁

- 结构化指标开启前后，原有核心统计必须一致。
- 同一实验在并行度 1 和 4 下结果一致。
- 同一 Trace 重复运行的模拟统计稳定。
- 每个图表都能回溯到原始结果和配置。
- 失败的运行不能从报告中静默消失。

---

## 9. 测试计划

### 9.1 C++ 单元测试

- `MetricRecord` 字段和序列化。
- `MetricSink` 写入顺序。
- 关闭指标时无额外行为。
- 预取 issue、fill 和 demand use 时间采集。
- 驱逐、失效和未使用路径。
- 多缓存和多 CPU 作用域。

### 9.2 Python 单元测试

- 实验矩阵展开。
- 并行度、超时和重试。
- JSON 解析和非法输入。
- 指标公式和除零保护。
- 基线比较和聚合。
- ChartSpec 到输出文件的映射。

### 9.3 集成测试

- 用短 Trace 运行 `no` 和 `next_line`。
- 检查原始 JSON、解析结果、比较结果和图表的完整链路。
- 检查并行度 1 和 4 的一致性。
- 检查失败任务对最终报告的影响。

### 9.4 回归测试

- ChampSim 原有 `make test`。
- Python 配置测试。
- 结构化输出新增后，文本输出和 JSON 输出保持兼容。
- 指标关闭时，运行结果与原始基线一致。

---

## 10. 阶段计划

### 第 3 周：选题定稿

产物：

- 一页选题说明；
- 目标功能与非目标；
- 初步实验对象；
- 风险清单。

验收：

- 主线只包含结构化指标、批量实验、比较、图表和新增预取指标；
- 不把完整一致性协议或复杂 GUI 纳入第一版。

### 第 4 周：基线准备

产物：

- 可构建的 ChampSim；
- 可运行 Trace；
- 原始 JSON 结果；
- 构建和运行说明；
- 基线脚本。

验收：

- 至少跑通一次 Warmup + Simulation；
- 保存原始输出；
- 知道完整实验预计耗时。

### 第 5 周：系统分析

产物：

- 模块划分；
- 核心对象图；
- 结构化指标 JSON 设计；
- 实验平台类图初稿。

验收：

- `CACHE`、`O3_CPU`、`phase_stats`、`json_printer` 的依赖关系清楚；
- 明确哪些功能放 C++，哪些放实验平台。

### 第 6 周：最小实验闭环

产物：

- `ExperimentSpec`；
- `RunSpec`；
- 单任务执行；
- 原始结果目录结构；
- JSON 解析对象。

验收：

- 能定义一个预取器和一个 Trace；
- 能自动运行并读取结果。

### 第 7 周：结构化指标闭环

产物：

- `MetricDefinition`；
- `MetricRecord`；
- `MetricRegistry`；
- JSON/JSONL 输出；
- 派生指标计算。

验收：

- 至少输出 IPC、MPKI、预取准确率、预取覆盖率；
- 指标具有单位、公式和作用域。

### 第 8-9 周：初版报告与第一次研讨

产物：

- 系统分析与设计报告初版；
- 第一次研讨 PPT；
- 需求、对象、接口、时序、测试计划和风险；
- 可运行的最小实验版本。

验收：

- 报告结合真实代码和真实运行结果；
- 不只展示大模型生成的分析；
- 明确下一阶段新增指标和批量实验设计。

### 第 10 周：并行运行

产物：

- `JobPlanner`；
- `ParallelRunScheduler`；
- `RunExecutor`；
- 独立工作目录；
- 超时、失败和重试机制。

验收：

- 至少并行运行 3 个任务；
- 并行度 1 和 4 的模拟结果一致；
- 日志和 JSON 无覆盖。

### 第 11 周：自动比较

产物：

- `ResultSet`；
- `Aggregator`；
- `BaselineSelector`；
- `Comparator`；
- `ComparisonResult`。

验收：

- 能比较 no 和 next_line；
- 能输出相对 IPC 变化；
- 能处理缺失数据和除零。

### 第 12 周：图表与报告

产物：

- 柱状图；
- 折线图；
- 热力图；
- HTML/Markdown 报告；
- 图表数据回溯。

验收：

- 至少 3 类图；
- 每张图对应 `run_id`、Trace、配置和原始结果；
- 图表直接从结果对象生成。

### 第 13 周：第二次提交与初步实现

产物：

- 批量实验可运行版本；
- 单元测试和集成测试；
- 初步实验数据；
- 失败方案和迭代记录；
- 第二次提交报告/材料。

验收：

- 至少完成 2 个预取器和 2 条 Trace 的对照实验；
- 所有核心路径有测试；
- 没有核心功能停留在 TODO 或 `NotImplementedError`。

### 第 14 周：新增预取指标实现

产物：

- `PrefetchTimelinessCollector`；
- issue、fill、demand use 记录；
- lead time 和 timely rate；
- 边界测试。

验收：

- 合成 Trace 下人工计算值与程序输出一致；
- 指标关闭时不改变原有统计；
- 指标开启时仿真统计不被影响。

### 第 15 周：完整实验

产物：

- 多预取器、多 Trace、多次重复数据；
- 聚合结果；
- 性能对比图；
- 原始结果归档。

验收：

- 实验配置完全可复现；
- 结果统计有均值和波动范围；
- 失败任务有处理记录。

### 第 16 周：分析结果与消融

产物：

- 新指标与传统准确率/覆盖率的关联分析；
- 至少一个消融或对照实验；
- 对异常结果的解释。

验收：

- 能回答“新指标是否解释了原有指标无法解释的现象”；
- 不只展示一张“更好看”的图。

### 第 17 周：最终报告和答辩准备

产物：

- 最终报告；
- 答辩 PPT；
- 演示脚本；
- 复现实验命令；
- 大模型使用与核验记录。

验收：

- 实际实现、测试结果和报告描述一致；
- 无未说明的占位代码；
- 所有数字可复现。

### 第 18 周：最终提交

产物：

- 最终报告；
- 最终代码；
- 测试结果；
- 图表和原始数据；
- 运行说明；
- PPT 和演示材料。

验收：

- 从干净环境按 README 可完成配置、编译和最小实验；
- 源码仓库包含完整提交历史和阶段材料。

---

## 11. 工作包拆分

| 工作包 | 内容 | 依赖 | 核心产物 |
| --- | --- | --- | --- |
| WP0 | 构建基线与 Trace 选择 | 无 | 基线数据、运行命令 |
| WP1 | 统计模型与 JSON Schema | WP0 | MetricRecord、结果 Schema |
| WP2 | 批量运行与并行调度 | WP1 | ExperimentSpec、Runner |
| WP3 | 解析、聚合和比较 | WP1、WP2 | ResultSet、Comparator |
| WP4 | 图表和报告 | WP3 | ChartSpec、Renderer |
| WP5 | 预取提前量指标 | WP0、WP1 | Collector、指标测试 |
| WP6 | 测试与实验 | WP2-WP5 | 测试报告、实验数据 |
| WP7 | 报告和答辩 | 全部 | 报告、PPT、演示 |

---

## 12. 面向对象设计重点

### 12.1 必须体现的 OOP 概念

- 封装：Runner 封装进程管理，Comparator 封装比较规则，Renderer 封装图形库。
- 抽象：定义 `RunExecutor`、`ResultParser`、`MetricDefinition`、`Renderer` 等接口。
- 多态：新增执行器、解析器、指标或渲染器无需修改调用方。
- 组合：实验平台组合调度器、仓库、比较器和渲染器，而不是使用深层继承。
- 生命周期：管理进程、任务状态、结果文件、图表对象和临时目录。
- 异常处理：超时、进程失败、坏 JSON、缺失指标和配置不一致。

### 12.2 设计模式使用建议

| 场景 | 模式 | 目的 |
| --- | --- | --- |
| 构造实验配置 | Builder | 分离复杂配置和对象表示 |
| 创建解析器/执行器/渲染器 | Factory | 根据配置选择实现 |
| 计算不同指标 | Strategy | 指标算法可替换 |
| 多种图表类型 | Strategy | 渲染逻辑与数据解耦 |
| 任务状态通知 | Observer | 显示排队、运行和完成状态 |
| 历史结果管理 | Repository | 统一访问运行结果 |
| 不同输出格式 | Adapter | 兼容 JSON/文本/历史 Schema |
| 固定实验执行流程 | Template Method | 固化计划、执行、解析和保存步骤 |

不要为了展示模式而增加无意义层级。报告必须说明“没有这个抽象会导致什么问题”。

---

## 13. 最终报告结构建议

结合参考报告的优点，建议最终报告采用以下结构：

1. 选题背景与研究问题
2. ChampSim 系统理解与对象分析
3. 需求分析与用例
4. 总体架构和对象职责
5. 设计模式及引入理由
6. 结构化指标输出实现
7. 多预取器/多 Trace 并行实验
8. 自动比较、图表和报告
9. 新增预取提前量指标
10. 测试、正确性与性能验证
11. 实验结果与分析
12. 方案迭代、失败尝试与原因
13. 大模型使用与人工核验
14. 局限性与未来工作
15. 结论与复现说明

---

## 14. 两次课堂研讨安排

### 第一次研讨：第 8-9 周

重点：

- ChampSim 系统理解；
- 现有输出和指标缺口；
- 对象、职责和接口；
- 批量实验与可视化方案；
- 新增预取指标的研究问题；
- 测试计划、风险和里程碑。

演示内容：

- 一张 ChampSim 对象图；
- 一张实验平台数据流图；
- 一个最小运行闭环；
- 初步 JSON 结果。

### 第二次研讨：第 15-18 周

重点：

- 结构化指标实现；
- 并行运行和结果一致性；
- 自动比较和图表；
- 新指标正确性；
- 多预取器/多 Trace 结果；
- 失败方案和修正过程。

演示内容：

- 完整批量实验命令；
- 原始结果、比较结果和报告；
- 图表；
- 新指标验证；
- 一次失败重试或消融实验。

---

## 15. 交付物验收清单

### 代码

- [ ] ChampSim 可正常构建。
- [ ] 原测试通过。
- [ ] 结构化指标可开关。
- [ ] 指标关闭不改变原统计。
- [ ] 多任务可并行运行。
- [ ] 每个任务输出独立。
- [ ] 结果可解析为对象。
- [ ] 基线比较可用。
- [ ] 图表可自动生成。
- [ ] 新增预取指标有真实埋点。
- [ ] 无核心 TODO/NotImplementedError。

### 测试

- [ ] C++ 单元测试。
- [ ] Python 单元测试。
- [ ] 端到端短 Trace 测试。
- [ ] 并行度一致性测试。
- [ ] 指标公式测试。
- [ ] 失败和超时路径测试。

### 实验

- [ ] 至少 3 个预取器。
- [ ] 至少 3 条 Trace。
- [ ] 至少 3 次重复。
- [ ] 结果均值和中位数。
- [ ] 原始结果和图表可追溯。
- [ ] 新指标有对照实验。

### 报告

- [ ] 需求可追踪。
- [ ] 类图与时序图完整。
- [ ] 设计模式有使用理由。
- [ ] 测试与实现一致。
- [ ] 性能数据可复现。
- [ ] 记录失败方案。
- [ ] 说明大模型使用范围。

---

## 16. 风险与应对

| 风险 | 影响 | 应对 |
| --- | --- | --- |
| 指标埋点影响仿真结果 | 研究结论不可信 | 关闭/开启指标做差分测试 |
| 记录表无限增长 | 内存和性能恶化 | 使用有界表并在使用/驱逐后清理 |
| 多进程并发写冲突 | 结果覆盖 | 每任务独立目录，原子写文件 |
| 不同配置不公平 | 比较结论错误 | 固定 Trace、warmup、simulation 和编译选项 |
| 图表和原始数据不一致 | 报告不可复现 | 只从 ResultSet 渲染 |
| 功能过多 | 无法按时完成 | 严格执行五个模块，拒绝范围扩张 |
| 报告超前于代码 | 答辩无法演示 | 每个里程碑必须可运行 |
| AI 生成代码错误 | 隐藏缺陷 | 测试、代码审查和来源记录 |
| 只用一条 Trace | 结论泛化差 | 至少 3 条不同负载 |

---

## 17. 最近两周立即执行的任务

### 第 1 周

1. 确认 ChampSim 构建环境和依赖。
2. 选择一组可在当前机器运行的短 Trace。
3. 保存原始 `--json` 输出。
4. 定义 `MetricRecord` 和 `MetricDefinition`。
5. 写出实验平台类图和数据流图。
6. 建立 `ExperimentSpec` JSON 草案。

### 第 2 周

1. 实现单任务 Runner。
2. 实现结果目录和元数据记录。
3. 实现 JSON 解析和 `RunResult`。
4. 计算 IPC、MPKI、预取准确率和覆盖率。
5. 运行 `no` 与 `next_line` 的最小对比。
6. 形成第一版报告和 PPT 所需图表。

这两周结束时，必须至少演示：

```text
一个配置
  -> 自动运行
  -> 生成原始结果
  -> 解析为 RunResult
  -> 输出一张对比图
```

该最小闭环完成后，再扩展多预取器、多 Trace 和预取提前量指标。



