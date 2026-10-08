# ChampSim 系统分析与设计报告（第 7-8 周初版）

## 面向微架构研究的结构化指标、并行实验与可视化平台

```text
课程：面向对象程序设计
项目：ChampSim 微架构模拟器
学生：[姓名]
学号：[学号]
日期：2026-10-08
版本：v0.1 初版
```

---

## 目录

1. 修订说明
2. 项目背景与目标
3. 需求分析
4. ChampSim 系统理解与模块分析
5. 核心流程设计
6. 总体架构与对象设计
7. 扩展点一：结构化指标输出
8. 扩展点二：多预取器与多 Trace 并行实验
9. 扩展点三：自动比较、图表与报告
10. 扩展点四：新增预取提前量指标
11. 数据与接口契约
12. 测试、验收标准与风险
13. 实施计划与交付物
14. 当前完成情况
15. 大模型使用与核验原则
16. 参考资料

---

## 1. 修订说明

本报告是 ChampSim 大作业的系统分析与设计初版，目标是完成问题定义、系统理解、需求分析、对象建模、接口设计、实验方案和风险分析。

本版本不声称已经完成全部扩展功能。当前已经完成的实际工作包括：

- Docker 构建环境；
- ChampSim 配置生成与编译；
- C++ 和 Python 测试；
- DPC-3 `429.mcf-217B` Trace 的下载与完整性校验；
- 一次小规模 `429.mcf` 基线仿真。

尚未完成的内容包括：

- 结构化指标 Schema；
- C++ 侧指标记录与输出；
- 多预取器、多 Trace 的批量执行器；
- 自动比较和图表生成；
- 预取提前量指标的完整埋点与实验验证。

---

## 2. 项目背景与目标

### 2.1 项目背景

ChampSim 是一个基于 Trace 的开源微架构模拟器，可以模拟乱序处理器、缓存层次、分支预测、数据预取、缓存替换、页表遍历和 DRAM 系统。

ChampSim 已经可以完成单次仿真并输出文本或 JSON 统计，但面向硬件研究时，仍然存在以下问题：

1. 不同预取器和 Trace 需要手工构建配置并重复执行。
2. 每次运行的命令、配置、日志和结果缺少统一管理。
3. 原始 JSON 统计需要额外脚本整理和比较。
4. 多方案、多次重复实验的手工对比成本较高。
5. 部分研究问题需要现有统计无法直接回答的新指标。
6. 自定义预取器统计通常直接打印，难以参与结构化分析和可视化。
7. 图表和结论与原始运行数据之间缺少可追踪关系。

### 2.2 项目目标

本项目的目标是构建一条可复现的微架构研究实验闭环：

```text
实验定义
  -> 多预取器/多 Trace 任务展开
  -> 有界并行运行
  -> 原始结果归档
  -> 结构化解析
  -> 指标计算
  -> 自动比较
  -> 图表和报告
  -> 结果追溯与复现
```

具体目标包括：

- 统一输出 ChampSim 指标；
- 支持多预取器和多 Trace 并行实验；
- 自动比较基线与优化方案；
- 自动生成研究图表和报告；
- 新增一个预取提前量研究指标；
- 保持 ChampSim 原有仿真语义和输出兼容性；
- 使用面向对象方法建立清晰的实验平台对象模型。

### 2.3 项目范围约束

第一版明确不包含：

- 完整操作系统模拟；
- 完整 Cache Coherence Protocol；
- 功耗和温度模型；
- 分布式集群调度；
- 图形化桌面应用；
- 自动生成研究结论；
- 运行时动态加载任意 C++ 策略模块。

多个预取器如果同时作用在同一个缓存中，只能作为预取器交互实验，不能与分别独立运行的性能结果直接混合比较。

---

## 3. 需求分析

### 3.1 功能性需求

| 编号 | 需求 | 说明 | 验证方式 |
| --- | --- | --- | --- |
| F-01 | 结构化指标输出 | 将 CPU、Cache、DRAM 和新增指标输出为统一 JSON/JSONL | Schema 校验、结果解析测试 |
| F-02 | 指标定义 | 每个指标具有 ID、单位、公式、作用域和阶段 | MetricRegistry 单元测试 |
| F-03 | 实验配置 | 定义预取器、Trace、重复次数和并行度 | 配置解析测试 |
| F-04 | 任务展开 | 将实验矩阵展开为多个独立 RunSpec | JobPlanner 单元测试 |
| F-05 | 并行执行 | 使用有界并行运行多个独立 ChampSim 进程 | 并行度 1/4 结果一致性测试 |
| F-06 | 结果归档 | 保存命令、配置、日志、退出码和原始 JSON | ArtifactStore 集成测试 |
| F-07 | 结果解析 | 将原始结果解析为 RunResult 和 ResultSet | JSON 解析测试 |
| F-08 | 自动比较 | 计算基线差异、相对变化和加速比 | Comparator 单元测试 |
| F-09 | 图表输出 | 生成柱状图、折线图、热力图和报告 | 图表文件与数据回溯测试 |
| F-10 | 新增预取指标 | 记录预取 issue、fill 和 demand use 时间 | 合成场景测试 |
| F-11 | 失败处理 | 支持超时、失败、重试和重跑 | 故障注入测试 |
| F-12 | 命令行接口 | 提供实验运行、比较和报告命令 | 端到端测试 |

### 3.2 非功能性需求

| 编号 | 需求 | 目标 |
| --- | --- | --- |
| NFR-01 | 正确性 | 指标关闭时，原有仿真统计必须保持一致 |
| NFR-02 | 可复现性 | 保存配置哈希、命令、Trace 和版本信息 |
| NFR-03 | 确定性 | 相同输入在不同并行度下产生相同统计结果 |
| NFR-04 | 可扩展性 | 新增指标或图表不修改运行器和解析器 |
| NFR-05 | 性能 | 指标和任务管理不能显著影响仿真时间 |
| NFR-06 | 可测试性 | 核心对象能够在不运行完整 Trace 的情况下测试 |
| NFR-07 | 兼容性 | 保留原有文本和 JSON 输出 |
| NFR-08 | 可维护性 | 仿真、解析、比较和渲染职责分离 |

### 3.3 主要用例

#### UC-01：定义实验

用户指定：

```text
预取器集合
Trace 集合
重复次数
Warmup 和 Simulation 指令数
并行度
基线方案
```

系统生成多个唯一 RunSpec。

#### UC-02：运行实验

系统按照资源策略执行任务，为每个任务创建独立目录，并保存：

```text
command.txt
config.json
stdout.log
stderr.log
result.json
metadata.json
```

#### UC-03：比较结果

用户选择基线，系统计算：

- IPC 变化；
- MPKI 变化；
- 预取准确率；
- 预取覆盖率；
- 预取提前量；
- 运行时间变化。

#### UC-04：生成报告

系统从 ComparisonResult 和 ResultSet 生成图表和 HTML/Markdown 报告。图表不直接读取原始 stdout。

#### UC-05：新增研究指标

在选定的缓存层级启用预取提前量记录，输出：

- 平均提前量；
- 中位提前量；
- P90 提前量；
- 及时预取比例；
- 迟到预取比例。

---

## 4. ChampSim 系统理解与模块分析

### 4.1 总体结构

ChampSim 可以分为以下层次：

```text
配置生成层
  -> 运行时编排层
  -> 逐周期组件层
  -> 处理器核心层
  -> 存储层次层
  -> Trace 输入层
  -> 策略模块层
  -> 统计输出层
  -> 基础工具层
```

### 4.2 配置生成层

主要代码：

```text
config.sh
config/parse.py
config/defaults.py
config/modules.py
config/instantiation_file.py
config/filewrite.py
config/makefile.py
```

职责：

- 解析 JSON；
- 合并配置；
- 推断默认值；
- 搜索模块；
- 生成 `generated_environment`；
- 生成 Makefile 片段。

关键结论：缓存拓扑和对象连接关系不是完全写死在 C++ 中，而是由 Python 配置生成。

### 4.3 运行时编排层

主要对象：

```text
champsim::environment
generated_environment
champsim::operable
```

核心流程：

```text
champsim::main
  -> do_phase
  -> do_cycle
  -> operable::operate_on
  -> operable::operate
```

不同组件可以具有不同时钟周期，`operate_on()` 将组件本地时间推进到全局时钟。

### 4.4 处理器核心层

主要对象：

```text
O3_CPU
RegisterAllocator
CacheBus
ooo_model_instr
LSQ_ENTRY
```

职责：

- 取指、译码、分派、调度、执行和退休；
- DIB；
- 寄存器重命名；
- Load/Store Queue；
- 分支预测和 BTB；
- 发起 L1I/L1D 访问。

### 4.5 存储层次

主要对象：

```text
CACHE
champsim::channel
cache_block
PageTableWalker
VirtualMemory
MEMORY_CONTROLLER
DRAM_CHANNEL
```

职责：

- 请求和响应队列；
- Tag Check；
- MSHR 合并；
- Fill、Eviction 和 Writeback；
- 数据预取；
- 缓存替换；
- 页表遍历；
- DRAM Bank、Row 和 Refresh 调度。

### 4.6 策略模块

| 模块 | 当前实现 |
| --- | --- |
| 分支预测 | `bimodal`、`gshare`、`perceptron`、`hashed_perceptron` |
| BTB | `basic_btb` |
| 预取器 | `no`、`next_line`、`ip_stride`、`spp_dev`、`va_ampm_lite` |
| 替换策略 | `lru`、`random`、`drrip`、`ship`、`srrip` |

策略模块主要通过编译期模板和接口适配器接入。

### 4.7 统计输出

当前统计对象：

```text
cpu_stats
cache_stats
cache_queue_stats
dram_stats
phase_stats
```

当前输出对象：

```text
plain_printer
json_printer
Heartbeat
```

当前输出的主要不足：

- 统计字段与具体 C++ 结构耦合；
- JSON 字段固定；
- 没有统一 MetricRecord；
- 没有运行级和实验级 ID；
- 自定义模块统计可能直接打印到屏幕；
- 缺少时间相关指标。

---

## 5. 核心流程设计

### 5.1 单次 ChampSim 运行

```text
读取 Trace
  -> 构造 generated_environment
  -> initialize
  -> Warmup phase
  -> Simulation phase
  -> 每周期推进 operable
  -> end_phase 汇总统计
  -> plain_printer / json_printer
```

### 5.2 批量实验流程

```text
ExperimentSpec
  -> JobPlanner
  -> RunSpec[]
  -> ParallelRunScheduler
  -> RunExecutor
  -> ChampSim
  -> RunArtifact
  -> ResultParser
  -> RunResult
  -> ResultSet
  -> Comparator
  -> ComparisonResult
  -> ChartSpec
  -> Renderer
  -> ReportBuilder
```

### 5.3 指标采集流程

```text
仿真事件
  -> MetricCollector
  -> MetricRecord
  -> MetricSink
  -> JSON/JSONL
  -> ResultParser
  -> MetricRegistry
  -> MetricCalculator
```

### 5.4 预取提前量流程

```text
预取发出
  -> 记录 issue time
  -> 预取填充完成
  -> 记录 fill time
  -> Demand 第一次使用
  -> 记录 demand use time
  -> lead_time
  -> 聚合统计
```

---

## 6. 总体架构与对象设计

### 6.1 分层架构

```text
+------------------------------------------------------+
| ChampSim 仿真核心                                     |
| O3_CPU / CACHE / PTW / DRAM / 策略模块                |
+------------------------------------------------------+
                    |
                    | MetricSink / JSONL
                    v
+------------------------------------------------------+
| 结构化指标层                                          |
| MetricDefinition / MetricRecord / MetricSink           |
+------------------------------------------------------+
                    |
                    | result.json / metrics.jsonl
                    v
+------------------------------------------------------+
| 实验编排层                                            |
| ExperimentSpec / JobPlanner / RunExecutor              |
+------------------------------------------------------+
                    |
                    v
+------------------------------------------------------+
| 解析与比较层                                          |
| ResultParser / RunResult / ResultSet / Comparator      |
+------------------------------------------------------+
                    |
                    v
+------------------------------------------------------+
| 可视化与报告层                                        |
| ChartSpec / Renderer / ReportBuilder                   |
+------------------------------------------------------+
```

### 6.2 关键对象职责

| 对象 | 职责 |
| --- | --- |
| `ExperimentSpec` | 描述实验矩阵、基线和约束 |
| `RunSpec` | 描述一次独立运行 |
| `JobPlanner` | 展开预取器、Trace 和重复次数 |
| `ParallelRunScheduler` | 控制并行、排队、超时和重试 |
| `RunExecutor` | 启动并监控 ChampSim 进程 |
| `RunArtifact` | 保存原始日志、JSON 和元数据 |
| `ResultParser` | 将原始输出转换为 RunResult |
| `RunResult` | 一次运行的结构化结果 |
| `ResultSet` | 多次运行结果集合 |
| `MetricDefinition` | 指标元数据 |
| `MetricRecord` | 单条指标记录 |
| `MetricRegistry` | 注册和查找指标 |
| `Comparator` | 基线比较和变化计算 |
| `ChartSpec` | 描述图表语义 |
| `Renderer` | 生成图表 |
| `ReportBuilder` | 组织实验结果和报告 |

### 6.3 依赖原则

- 仿真核心不依赖实验平台；
- Parser 不依赖 Scheduler；
- Comparator 不读取 stdout；
- Renderer 不启动 ChampSim；
- 图表只消费结构化结果；
- C++ 和 Python 之间通过 JSON/JSONL 解耦。

---

## 7. 扩展点一：结构化指标输出

### 7.1 问题

当前 `json_printer` 能输出固定统计，但缺少统一指标语义，也不适合自定义指标、长期结果存储和自动分析。

### 7.2 设计

新增：

```text
MetricDefinition
MetricRecord
MetricSink
JsonlMetricSink
MetricSchemaVersion
```

### 7.3 指标记录格式

```json
{
  "schema_version": "1.0",
  "run_id": "run-0001",
  "metric_id": "ipc",
  "scope": "cpu0",
  "phase": "roi",
  "value": 1.24,
  "unit": "instructions_per_cycle"
}
```

### 7.4 验收标准

- 指标可结构化输出；
- 原有文本和 JSON 输出保持兼容；
- 指标关闭时不改变仿真结果；
- 指标具有单位、公式和作用域；
- 指标可被 Python 解析和比较。

---

## 8. 扩展点二：多预取器与多 Trace 并行实验

### 8.1 问题

手工运行多个预取器和多条 Trace 容易产生：

- 命令不一致；
- 配置覆盖；
- 输出文件冲突；
- 失败任务被忽略；
- 无法批量复现。

### 8.2 设计

实验矩阵：

```text
(预取器, Trace, 配置, 重复编号) -> RunSpec
```

一个 RunSpec 对应一个独立 ChampSim 进程。

### 8.3 并行策略

- 有界进程池；
- 每任务独立工作目录；
- 每个任务独立日志；
- 原子写入结果；
- 超时、失败和重试状态明确；
- 并行度变化不影响统计结果。

### 8.4 验收标准

- 支持至少 3 个预取器和 3 条 Trace；
- 支持至少 3 次重复；
- 并行度 1 和 4 的结果一致；
- 失败任务可在报告中定位和重新执行。

---

## 9. 扩展点三：自动比较、图表与报告

### 9.1 比较内容

- 绝对变化；
- 相对变化；
- 加速比；
- 均值；
- 中位数；
- 标准差；
- 多次运行的波动。

### 9.2 图表类型

- 预取器对比柱状图；
- Trace 折线图；
- 预取器-Trace 热力图；
- 箱线图或误差条图。

### 9.3 报告输出

```text
results/
  run_id/
    command.txt
    config.json
    result.json
    metadata.json
    stdout.log
    stderr.log
  summary.csv
  comparison.json
  report.html
  figures/
```

### 9.4 验收标准

- 至少生成 3 类图表；
- 图表可从 `run_id` 回溯到原始结果；
- 渲染层不读取原始 stdout；
- 报告包含失败运行和数据完整性问题。

---

## 10. 扩展点四：新增预取提前量指标

### 10.1 研究问题

现有预取统计可以回答“预取有没有被使用”，但不能回答“预取是否及时”。

### 10.2 定义

```text
lead_time = first demand use time - prefetch fill time
```

### 10.3 输出指标

```text
mean_prefetch_lead_time
median_prefetch_lead_time
p90_prefetch_lead_time
timely_prefetch_rate
late_prefetch_rate
```

### 10.4 采集点

| 事件 | 位置 |
| --- | --- |
| 预取发出 | `CACHE::prefetch_line()` |
| 预取填充完成 | `CACHE::handle_fill()` |
| Demand 第一次使用 | `CACHE::try_hit()` 或命中路径 |

### 10.5 设计要求

- 只对启用的缓存层级采集；
- 记录表有界；
- 第一次使用后清理；
- 驱逐时记录过期；
- 关闭指标时不产生查找和内存增长；
- 多预取器需要记录来源。

### 10.6 验收标准

- 合成场景结果与人工计算一致；
- 指标开启和关闭时仿真统计一致；
- 迟到、及时、未使用和驱逐路径均有测试。

---

## 11. 数据与接口契约

### 11.1 ExperimentSpec

```json
{
  "experiment_id": "prefetch-study-001",
  "baseline": "no",
  "prefetchers": ["no", "next_line", "ip_stride", "spp_dev"],
  "traces": ["429.mcf-217B.champsimtrace.xz"],
  "repetitions": 3,
  "warmup_instructions": 1000000,
  "simulation_instructions": 2000000,
  "parallelism": 4
}
```

### 11.2 RunResult

```json
{
  "run_id": "run-0001",
  "trace": "429.mcf-217B.champsimtrace.xz",
  "prefetcher": "no",
  "repeat": 1,
  "status": "succeeded",
  "metrics": {
    "ipc": 0.756848,
    "prefetch_accuracy": 0.0,
    "prefetch_coverage": 0.0
  }
}
```

### 11.3 ComparisonResult

```json
{
  "baseline": "no",
  "variant": "ip_stride",
  "metric": "ipc",
  "absolute_delta": 0.05,
  "relative_delta": 0.066,
  "samples": 3
}
```

### 11.4 主要接口

```text
RunExecutor.execute(RunSpec) -> RunArtifact
ResultParser.parse(RunArtifact) -> RunResult
MetricDefinition.calculate(RunResult) -> MetricValue
Comparator.compare(ResultSet, baseline) -> ComparisonResult
Renderer.render(ChartSpec) -> Artifact
```

---

## 12. 测试、验收标准与风险

### 12.1 测试矩阵

| 测试类型 | 范围 |
| --- | --- |
| C++ 单元测试 | 指标结构、序列化、预取时间记录 |
| Python 单元测试 | 配置、任务展开、解析、比较、渲染 |
| 集成测试 | 短 Trace 运行和完整结果链路 |
| 差分测试 | 指标开启/关闭后的仿真统计 |
| 并发测试 | 并行度 1/4/8 的结果一致性 |
| 异常测试 | 超时、进程失败、非法 JSON、缺失字段 |
| 指标测试 | 合成场景下人工结果与程序结果一致 |

### 12.2 验收标准

功能：

- 可运行单次和批量实验；
- 可解析结构化结果；
- 可自动比较和生成图表；
- 可记录预取提前量。

正确性：

- 指标关闭时原有结果一致；
- 并行运行结果稳定；
- 图表可回溯到原始数据。

工程：

- 核心模块有单元测试；
- README 可从干净环境完成构建和最小实验；
- 没有未说明的占位实现。

### 12.3 风险

| 风险 | 影响 | 应对 |
| --- | --- | --- |
| 指标埋点改变仿真结果 | 研究结论不可信 | 开启/关闭差分测试 |
| 预取记录表无限增长 | 内存和性能恶化 | 使用有界表并清理 |
| 并发写冲突 | 结果被覆盖 | 每运行独立目录和原子写入 |
| Trace 配置不一致 | 比较不公平 | 固定配置、Trace 和编译版本 |
| 范围过大 | 无法按时完成 | 严格执行四个扩展点 |
| 报告超前于代码 | 答辩无法演示 | 每个里程碑必须可运行 |
| 图表与原始数据不一致 | 无法复现 | 只从 ResultSet 渲染 |

---

## 13. 实施计划与交付物

### 13.1 里程碑

| 阶段 | 工作 | 产物 |
| --- | --- | --- |
| 第 3 周 | 选题定稿 | 一页选题和范围 |
| 第 4 周 | 构建基线 | Docker 环境、Trace、基线结果 |
| 第 5 周 | 系统分析 | 类图、时序图、Schema |
| 第 6 周 | 单任务闭环 | Runner、ResultParser、RunResult |
| 第 7 周 | 结构化指标 | MetricRecord、MetricRegistry |
| 第 8-9 周 | 初版报告和第一次研讨 | 本报告、PPT、原型 |
| 第 10-12 周 | 并行运行、比较和图表 | 批量实验和报告生成 |
| 第 13 周 | 初步实现 | 第二次提交 |
| 第 14-16 周 | 新指标和完整实验 | 提前量指标和结果分析 |
| 第 17-18 周 | 最终报告和答辩 | 最终代码、报告和演示 |

### 13.2 交付物

- 最终系统分析与设计报告；
- ChampSim 指标扩展代码；
- 实验平台代码；
- Trace 和实验配置；
- 原始结果、比较结果和图表；
- C++/Python 测试；
- 运行和复现说明；
- 两次课堂汇报材料；
- 大模型使用与核验记录。

---

## 14. 当前完成情况

### 14.1 Docker 环境

已经完成：

```text
镜像：champsim-dev:ubuntu22.04
Ubuntu：22.04.5 LTS
GCC：11.4.0
CMake：3.22.1
Make：4.3
Python：3.10.12
vcpkg：2026-09-26
```

### 14.2 构建与测试

已经完成：

```text
vcpkg install 成功
config.sh 成功
make 成功
make test 成功
make pytest 成功
```

测试结果：

```text
C++：11388 assertions in 624 test cases
Python：228 passed, 1 skipped
```

### 14.3 429.mcf 基线

Trace：

```text
429.mcf-217B.champsimtrace.xz
SHA256：0f65748860e9f83468f0aef707f376681bdcbd9e056233ae039cb1be8ec88541
```

仿真：

```text
Warmup：1000000 instructions
Simulation：2000000 instructions
ROI instructions：2000004
ROI cycles：2642543
IPC：0.756848
Wall time：15.394851859 seconds
```

### 14.4 当前结论

当前已经完成构建环境和基线仿真，可以开始设计结构化指标和实验平台。

尚未完成核心扩展功能，因此本报告只能声明“分析和设计初版”，不能声明“系统扩展已经完成”。

---

## 15. 大模型使用与核验原则

本项目允许使用大模型辅助代码理解、方案设计、实现和测试，但必须遵守：

- 大模型生成的代码必须经过编译和测试；
- 大模型生成的分析必须与真实源码核对；
- 性能数据必须由实际运行产生；
- 不把尚未运行过的代码写成已完成；
- 不把计划中的功能写成实际成果；
- 记录被拒绝的方案和原因；
- 最终设计决策由项目作者负责。

---

## 16. 参考资料

1. ChampSim 官方仓库与文档。
2. N. Gober et al., The Championship Simulator: Architectural Simulation for Education and Competition.
3. DPC-3 ChampSim Trace 集合：
   https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/
4. DPC-3 429.mcf-217B Trace：
   https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/429.mcf-217B.champsimtrace.xz
5. 课程大作业说明。
6. 参考课程报告的系统分析与设计组织方式。

---

## 附录 A：核心对象关系

```text
generated_environment
  +-- channels
  +-- DRAM
  +-- vmem
  +-- ptws
  +-- caches
  +-- cores

O3_CPU
  +-- L1I_bus
  +-- L1D_bus
  +-- branch predictor
  +-- BTB

CACHE
  +-- channel queues
  +-- MSHR
  +-- blocks
  +-- prefetcher
  +-- replacement

PageTableWalker
  +-- upper_levels
  +-- lower_level
  +-- VirtualMemory
```

## 附录 B：建议的实现顺序

```text
1. MetricSchema
2. ExperimentSpec
3. 单任务 Runner
4. RunResult 和 ResultParser
5. MetricRegistry 和派生指标
6. 并行调度
7. Comparator
8. Renderer 和 ReportBuilder
9. PrefetchTimelinessCollector
10. 完整实验和最终报告
```

## 附录 C：初版验收检查

- [ ] Schema 和领域对象完成；
- [ ] 单任务 Runner 完成；
- [ ] 结果解析完成；
- [ ] IPC、MPKI、准确率和覆盖率可计算；
- [ ] 并行实验可运行；
- [ ] 自动比较完成；
- [ ] 至少三类图表完成；
- [ ] 预取提前量指标有测试；
- [ ] 所有数字可复现；
- [ ] 报告与代码状态一致。
