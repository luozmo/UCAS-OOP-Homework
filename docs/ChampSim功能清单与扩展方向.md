# ChampSim 功能清单与扩展方向分析

> 本文基于当前源码和配置生成结果，梳理 ChampSim 已经具备的功能，并给出一组适合课程大作业、工程开发和硬件研究方向继续扩展的思路。

---

## 1. 总体定位

ChampSim 是一个基于 Trace 的微架构模拟器，目标是快速开展以下研究：

- 分支预测；
- 乱序执行核心；
- 缓存层次和缓存管理；
- 数据预取；
- 缓存替换；
- 虚拟地址翻译和页表遍历；
- DRAM 地址映射和调度；
- 多核共享缓存和异构核心配置。

它的核心特点不是“完整模拟操作系统和程序执行”，而是：

```text
Trace
  -> 指令模型
  -> 乱序核心
  -> 缓存层次
  -> 页表遍历
  -> DRAM
  -> 统计结果
```

---

## 2. 当前已有功能

## 2.1 构建与配置功能

### 已有能力

- 使用 JSON 文件描述微架构配置。
- 支持多个 JSON 配置进行组合。
- 支持 `--join chain` 和 `--join product` 两种组合方式。
- 支持自动推断缺失的缓存层级和默认参数。
- 支持选择内置或外部模块目录。
- 支持配置多个构建变体和可执行文件名称。
- 通过 Python 生成 C++ 对象图。
- 通过 Python 生成编译所需的 Makefile 片段。
- 支持只编译指定模块或编译所有模块。

### 主要代码位置

```text
config.sh
config/parse.py
config/defaults.py
config/modules.py
config/instantiation_file.py
config/filewrite.py
config/makefile.py
```

### 配置可控制的对象

```text
CPU
DIB
L1I / L1D
L2C
ITLB / DTLB / STLB
LLC
PTW
物理内存
虚拟内存
缓存替换策略
数据预取器
分支预测器
BTB
```

---

## 2.2 处理器核心功能

### 已有能力

- 多核和异构核配置。
- 每个核心可配置频率和时钟周期。
- 可配置取指、译码、分派、调度、执行和退休宽度。
- 可配置 IFETCH、DECODE、DISPATCH、ROB、LQ、SQ 大小。
- 支持 Decoded Instruction Buffer（DIB）。
- 支持寄存器重命名和物理寄存器分配。
- 支持分支预测和 BTB。
- 支持 Load/Store Queue 的调度和依赖处理。
- 支持分支误预测惩罚和流水线停顿。
- 支持多核共享缓存。
- 支持 Warmup 和 Simulation 两个阶段。

### 当前核心流水线阶段

```text
initialize_instruction
check_dib
fetch_instruction
promote_to_decode
decode_instruction
dispatch_instruction
schedule_instruction
execute_instruction
operate_lsq
complete_inflight_instruction
retire_rob
```

### 已有分支模块

```text
bimodal
gshare
perceptron
hashed_perceptron
basic_btb
```

---

## 2.3 缓存和存储层次功能

### 已有能力

- 任意缓存层次和缓存连接关系。
- 可配置 Set、Way、MSHR、队列大小和延迟。
- 可配置 Tag Check 和 Fill 带宽。
- 支持 RQ、WQ、PQ 三类队列。
- 支持 MSHR 合并。
- 支持 Fill、Eviction、Writeback 和 Dirty Data 处理。
- 支持 Prefetch Queue 和 Prefetch Fill。
- 支持虚拟地址预取。
- 支持缓存替换策略。
- 支持缓存预取器。
- 支持多个缓存层级：L1I、L1D、L2C、LLC 和自定义缓存。
- 支持 TLB 和页表缓存路径。
- 支持多核共享 LLC。

### 已有预取器

```text
no
next_line
ip_stride
spp_dev
va_ampm_lite
```

### 已有替换策略

```text
lru
random
drrip
ship
srrip
```

### 缓存模块接口

```text
prefetcher_initialize
prefetcher_cache_operate
prefetcher_cache_fill
prefetcher_cycle_operate
prefetcher_branch_operate
prefetcher_final_stats

initialize_replacement
find_victim
update_replacement_state
replacement_cache_fill
replacement_final_stats
```

---

## 2.4 分支预测功能

### 已有能力

- 分支方向预测。
- 分支目标预测。
- 直接分支、间接分支、调用和返回地址预测。
- 返回地址栈。
- 分支类型分类。
- 分支误预测统计。
- 分支预测器可替换。
- BTB 可替换。
- 多分支预测器/BTB 模块组合接口。

### 主要统计

- 分支类型总数；
- 各类型误预测数；
- 误预测数；
- ROB 在误预测时的平均占用。

---

## 2.5 虚拟内存和页表功能

### 已有能力

- 多级页表配置。
- 可配置 PTE 页面大小。
- 可配置页表层级。
- 支持 Minor Page Fault 惩罚。
- 支持页表物理页分配。
- 支持 Address Space ID。
- 支持 Page Table Walker MSHR。
- 支持多级 PTW 流程。
- 支持 Page Structure Cache（PSCL）。
- 支持 TLB Miss 和页表查询路径。

### 主要对象

```text
VirtualMemory
PageTableWalker
ITLB
DTLB
STLB
```

---

## 2.6 DRAM 功能

### 已有能力

- 物理内存大小和通道配置。
- DRAM 地址映射。
- Channel、Rank、Bank Group、Bank、Row、Column 映射。
- Bank Group 和 Bank 并行访问。
- Row Buffer Hit/Miss 状态。
- Read/Write 模式切换。
- DRAM 刷新。
- Data Bus 占用和总线切换时间。
- 请求重排和 Bank 调度。

### 主要统计

- RQ/WQ Row Buffer Hit；
- RQ/WQ Row Buffer Miss；
- WQ Full；
- Refresh 次数；
- Data Bus 拥塞信息。

---

## 2.7 Trace 功能

### 已有能力

- 一次运行支持每个 CPU 一条 Trace。
- 多核运行支持多条 Trace。
- 支持重复读取 Trace。
- 支持普通二进制 Trace。
- 支持 gzip、xz 和 bzip2 压缩 Trace。
- 支持 CloudSuite Trace 格式。
- 支持批量读取和指令缓冲。
- 支持分支目标修正。
- 支持 Trace EOF 终止。

### 主要对象

```text
champsim::tracereader
champsim::bulk_tracereader
champsim::inf_istream
champsim::repeatable
ooo_model_instr
```

---

## 2.8 输出和统计功能

### 已有能力

- 文本统计输出。
- JSON 结构化输出。
- Warmup 和 Simulation 分区统计。
- ROI（Region of Interest）统计。
- CPU、Cache、DRAM 分类统计。
- 按访问类型统计命中、缺失和 MSHR 合并。
- 预取请求、发出、有效和无用统计。
- 平均缺失延迟统计。
- 分支类型和误预测统计。
- DRAM Row Buffer 和 Refresh 统计。
- Heartbeat 进度输出。

### 主要对象

```text
cpu_stats
cache_stats
cache_queue_stats
dram_stats
phase_stats
plain_printer
json_printer
Heartbeat
```

---

## 2.9 测试和工具功能

### 已有能力

- C++ 单元测试。
- Python 配置解析测试。
- 配置生成测试。
- 缓存、预取、替换、PTW、DRAM 和虚拟内存测试。
- Pin Trace 生成工具。
- CVP Trace 转换工具。
- 模块 mock 和缓存交互测试基础设施。

---

## 3. 当前功能边界

ChampSim 并不是一个完整系统模拟器，因此当前不直接提供：

- 操作系统和系统调用模拟；
- 完整程序加载和动态链接行为；
- 非内存指令的详细执行时序；
- 完整 Cache Coherence Protocol；
- 功耗、能量和温度模型；
- 多线程程序的动态调度模拟；
- 运行时动态加载预取器或替换策略；
- 内置批量实验编排；
- 内置多方案自动比较；
- 内置图形化可视化；
- 通用的结构化指标 Schema；
- 检查点保存和恢复；
- 自动参数搜索或机器学习调优。

这些边界正好对应下一节可以扩展的方向。

---

## 4. 扩展功能思路

## 4.1 方向一：实验平台与结果可视化

### 目标

把 ChampSim 从“单次运行并打印结果”扩展为“批量实验、统一解析、自动比较和可视化”的研究平台。

### 可扩展对象

```text
ExperimentSpec
RunSpec
JobPlanner
ParallelRunScheduler
RunExecutor
RunArtifact
ResultParser
RunResult
ResultSet
MetricRegistry
Comparator
ChartSpec
Renderer
ReportBuilder
```

### 可实现功能

- 一次定义多个预取器、替换策略、Trace 和配置。
- 并行运行多个独立 ChampSim 进程。
- 控制最大并行度、超时、重试和取消。
- 保存每次运行的命令、配置、日志、退出码和原始 JSON。
- 将文本或 JSON 解析为统一结果对象。
- 计算性能、缓存、预取、DRAM 和多核公平性指标。
- 自动生成对比表、柱状图、折线图、热力图和箱线图。
- 生成 HTML 或 Markdown 实验报告。
- 支持复现实验和失败重跑。
- 生成配置哈希和运行元数据。

### 适合课程的原因

- 对象职责明确；
- 包含并发、异常、解析、比较和渲染；
- 与现有 ChampSim 核心低耦合；
- 容易做单元测试和端到端展示。

---

## 4.2 方向二：结构化指标与自定义埋点

### 目标

让预取器、替换策略、缓存、PTW 和 DRAM 都能输出统一的、可自动分析的研究指标。

### 当前缺口

- 统计字段是具体 C++ 结构；
- JSON 输出依赖固定 `to_json`；
- 自定义预取器统计可能直接打印到屏幕；
- 缺少统一指标注册、指标单位和指标公式；
- 缺少统一运行 ID 和阶段 ID；
- 缺少时间序列或事件级指标。

### 可扩展对象

```text
MetricDefinition
MetricRegistry
MetricRecord
MetricSink
MetricCollector
SchemaValidator
```

### 可新增指标

#### 预取指标

```text
预取准确率 = useful prefetch / issued prefetch
预取浪费率 = useless prefetch / issued prefetch
预取覆盖率 = useful prefetch / demand misses
预取请求有效率 = issued prefetch / requested prefetch
预取及时性 = demand time - ready time
预取污染 = prefetch-caused eviction
预取占用 = PQ occupancy
预取带宽放大 = extra DRAM traffic
```

#### 缓存指标

```text
各级缓存 MPKI
按访问类型的命中率
MSHR 平均占用
MSHR Full 比例
RQ/WQ/PQ 平均占用
队列等待时间
缓存行生命周期
重用距离分布
Victim 重用距离
```

#### CPU 指标

```text
IPC
分支 MPKI
按分支类型 Accuracy
ROB 平均占用
LQ/SQ 平均占用
停顿来源分解
前端和后端阻塞比例
```

#### DRAM 指标

```text
Row Buffer Hit Rate
Bank Conflict
Row Conflict
Refresh Overhead
Read/Write Turnaround
Data Bus Utilization
请求平均等待时间
```

---

## 4.3 方向三：现有策略模块扩展

### 4.3.1 新数据预取器

可以新增：

- Adaptive Stride；
- Region-based Prefetching；
- Temporal Streaming；
- Graph-based Prefetching；
- Hybrid Prefetcher；
- L1/L2/LLC 协同预取；
- 带置信度控制的预取器；
- 带污染控制的预取器；
- 以不可预测性为依据的动态预取度；
- 使用 metadata 在缓存层级之间传递预取信息。

### 4.3.2 新替换策略

可以新增：

- Adaptive Replacement；
- Re-reference Interval Prediction；
- Bypass/Insertion Policy；
- Set Dueling；
- Cache Partitioning；
- Prefetch-aware Replacement；
- Replacement 与 Prefetcher 协同；
- 多核公平性感知替换。

### 4.3.3 新分支预测器

可以新增：

- TAGE 类预测器；
- Perceptron 变体；
- 多预测器组合；
- 按分支类型特化的预测器；
- 返回地址栈优化；
- 间接分支目标预测。

### 4.3.4 新 BTB

可以新增：

- Hybrid BTB；
- Region BTB；
- Multi-level BTB；
- 间接分支专用 BTB；
- 更细粒度的 BTB replacement。

---

## 4.4 方向四：PTW、TLB 和虚拟内存扩展

### 可扩展思路

- Huge Page 和多种页面大小。
- 多级 TLB 结构。
- Page Walk Cache 扩大和替换策略。
- 页表遍历预取。
- 多核共享 PTW。
- 地址翻译延迟建模。
- TLB Shootdown 和上下文切换模拟。
- 进程级地址空间和 ASID 扩展。
- 多级页表组合优化。
- Page Fault 类型细分和统计。

### 研究价值

虚拟内存和地址翻译对数据中心、虚拟化和大内存应用越来越重要，PTW 方向具有较高的研究价值。

---

## 4.5 方向五：DRAM 与内存控制器扩展

### 可扩展思路

- 将 DRAM 调度器抽象成策略模块。
- 新增 FR-FCFS、ATLAS、PARBS、BLISS 等调度策略。
- 增加 Bank Group 级调度策略。
- 增加 QoS-aware 调度。
- 增加读写优先级和饥饿保护。
- 增加 Memory-side Cache 或近内存计算模型。
- 增加 HBM、DDR5 或 3D 堆叠内存参数。
- 增加 Row Hammer 相关访问统计。
- 增加刷新延迟和 Bank Group 冲突分析。
- 增加地址映射随机化与地址映射策略比较。

### 当前设计机会

当前 `DRAM_CHANNEL::schedule_packet()` 是具体实现。如果把调度策略抽成抽象接口：

```cpp
class dram_scheduler {
public:
    virtual queue_iterator select(...) = 0;
    virtual void update(...) = 0;
};
```

就可以通过配置选择不同 DRAM 调度器。这与现有 Prefetcher 和 Replacement 模块的扩展方式一致。

---

## 4.6 方向六：多核、共享缓存和一致性扩展

### 可扩展思路

- 多核共享 LLC 争用分析。
- Per-core 和 per-shared-cache 指标。
- 公平性和 QoS 指标。
- Core-to-cache 亲和性映射。
- Cache Partitioning。
- Shared Cache Insertion Policy。
- 跨核预取和共享数据预取。
- 简化 Snooping 或 Directory Coherence Protocol。
- 多核同步和通信模型。
- Multi-programmed Workload 干扰分析。

### 风险

完整 Cache Coherence Protocol 属于较大的体系结构扩展，涉及状态机、消息传递、目录、MSHR、请求冲突和死锁处理，不适合把“完整一致性协议”作为课程最小版本。

---

## 4.7 方向七：功耗、能量和可靠性与安全研究

### 可扩展思路

- 缓存访问能量计数。
- DRAM 读写能量计数。
- 预取额外带宽的能耗代价。
- Bank 冲突和刷新能耗。
- 访问延迟和能量联合目标。
- Row Hammer 统计。
- Cache Side-channel 访问模式分析。
- Prefetcher 信息泄漏分析。
- 攻击者和受害者 Trace 对照实验。

### 风险

ChampSim 当前没有完整功耗参数和电路模型，因此功耗方向需要：

- 设定能量模型；
- 引入工艺参数；
- 校准结果；
- 明确模型假设。

这类方向研究价值高，但验证成本明显大于结构化输出或可视化扩展。

---

## 4.8 方向八：仿真内核和运行性能扩展

### 可扩展思路

- 优化 `do_cycle()` 中每周期视图构建和排序。
- 使用增量调度结构维护 `operable` 顺序。
- 降低 `channel` 和缓存请求复制。
- 优化 MSHR、响应和依赖向量。
- 优化 Trace 解码和批处理管线。
- 优化阶段统计复制。
- 减少不必要的虚函数调用。
- 增加并行实验执行，区分“模拟器内部并行”和“多个独立进程并行”。

### 注意

这类优化改变的是“主机侧运行时间”或“内存占用”，不应改变微架构统计。报告中必须把：

```text
host wall-clock time
```

和：

```text
simulated IPC / cache misses / DRAM stats
```

分开评价。

---

## 5. 扩展方向优先级

| 扩展方向 | 实现难度 | 研究价值 | 适合课程 | 建议 |
| --- | --- | --- | --- | --- |
| 结构化指标 Schema | 低 | 高 | 高 | 首选 |
| 批量运行和多 Trace 对比 | 中 | 高 | 高 | 首选 |
| 结果解析与可视化 | 中 | 高 | 高 | 首选 |
| 预取器/替换策略 | 中 | 高 | 高 | 适合硬件方向 |
| 自定义指标埋点 | 中 | 高 | 高 | 推荐与实验平台结合 |
| PTW/TLB 扩展 | 中高 | 高 | 中 | 适合有虚拟内存背景 |
| DRAM Scheduler 模块化 | 中高 | 高 | 中高 | 方向很有价值 |
| 仿真内核性能优化 | 中高 | 中 | 中高 | 依赖 profiling |
| 多核 QoS 和共享缓存 | 高 | 高 | 中 | 可作为扩展而不是主线 |
| 完整 Cache Coherence | 高 | 高 | 低 | 不建议课程首选 |
| 功耗和热模型 | 高 | 高 | 中 | 需要额外模型验证 |
| 安全/Side-channel | 高 | 高 | 中 | 需要攻击者和受害者建模 |

---

## 6. 推荐的可落地扩展组合

### 组合 A：课程最稳妥

```text
结构化统计输出
  + 批量运行多个预取器
  + 多 Trace 结果比较
  + 自动生成图表
```

交付物：

- `ExperimentSpec`；
- `RunSpec`；
- `ParallelRunScheduler`；
- `ResultParser`；
- `ResultSet`；
- `Comparator`；
- `Renderer`；
- 单元测试和端到端实验。

### 组合 B：实验平台加新指标

```text
组合 A
  + 预取准确率
  + 预取覆盖率
  + 预取及时性
  + 预取污染
```

重点是新增指标需要：

- 明确公式；
- 明确统计位置；
- 明确 warmup/ROI 阶段；
- 增加合成 Trace 测试。

### 组合 C：硬件研究型

```text
实现一个新预取器
  + 与 no / next_line / ip_stride / spp_dev 对比
  + 增加预取及时性和污染指标
  + 自动生成多 Trace 对比图
```

这个组合既包含新的微架构策略，也包含实验基础设施，适合展示硬件研究完整流程。

### 组合 D：内存系统研究型

```text
DRAM Scheduler 模块化
  + 实现一个新调度策略
  + 对比 FR-FCFS 和现有实现
  + 统计 Row Buffer、等待时间和公平性
  + 生成对比报告
```

该方向更偏微架构研究，工作量比实验平台方向大。

---

## 7. 建议新增的软件对象

### 仿真侧

```text
MetricRecord
MetricDefinition
MetricSink
MetricCollector
StatsSchema
```

### 实验侧

```text
ExperimentSpec
RunSpec
RunId
JobPlanner
ResourcePolicy
ParallelRunScheduler
RunExecutor
RunArtifact
RunResult
ResultSet
```

### 分析侧

```text
MetricRegistry
MetricCalculator
Aggregator
BaselineSelector
Comparator
ComparisonResult
```

### 输出侧

```text
ChartSpec
Renderer
BarChartRenderer
LineChartRenderer
HeatmapRenderer
ReportBuilder
ArtifactStore
```

---

## 8. 建议的扩展数据格式

### 运行配置

```json
{
  "experiment_id": "prefetch-compare-001",
  "baseline": "no",
  "prefetchers": ["no", "next_line", "ip_stride", "spp_dev"],
  "traces": ["trace-a", "trace-b", "trace-c"],
  "repetitions": 3,
  "warmup_instructions": 200000000,
  "simulation_instructions": 500000000,
  "parallelism": 4
}
```

### 单次运行结果

```json
{
  "run_id": "run-0001",
  "trace": "trace-a",
  "prefetcher": "ip_stride",
  "repeat": 1,
  "status": "succeeded",
  "metrics": {
    "ipc": 1.24,
    "l1d_mpki": 12.4,
    "prefetch_accuracy": 0.62,
    "prefetch_coverage": 0.38
  }
}
```

### 比较结果

```json
{
  "baseline": "no",
  "variant": "ip_stride",
  "metric": "ipc",
  "absolute_delta": 0.12,
  "relative_delta": 0.108,
  "samples": 3
}
```

---

## 9. 面向对象设计映射

| 需求 | 推荐设计手段 |
| --- | --- |
| 多种预取器配置 | Builder + Factory |
| 多种运行方式 | Strategy |
| 多种结果格式 | Adapter + Strategy |
| 多种图表 | Strategy + Renderer |
| 任务状态通知 | Observer |
| 实验结果管理 | Repository |
| 多次运行与对比 | Template Method + Composite |
| 指标计算 | Strategy + Registry |
| 资源限制和并行度 | Policy Object |

---

## 10. 分阶段实施建议

### 第一阶段：最小闭环

- 结构化收集现有指标；
- 支持一个预取器和一条 Trace；
- 支持结果解析；
- 输出一张柱状图；
- 增加单元测试。

### 第二阶段：批量实验

- 支持多个预取器；
- 支持多条 Trace；
- 支持重复运行；
- 支持有界并行；
- 保存运行元数据和失败记录。

### 第三阶段：研究指标

- 增加预取准确率、覆盖率和及时性；
- 增加不同缓存层级的指标；
- 增加指标公式和单位；
- 增加指标正确性测试。

### 第四阶段：可视化和报告

- 分组柱状图；
- Trace 折线图；
- 预取器-Trace 热力图；
- 箱线图；
- HTML/Markdown 报告。

### 第五阶段：硬件策略研究

- 增加新预取器或替换策略；
- 或多个 DRAM 调度策略；
- 与基线方案做系统对比；
- 记录成功和失败思路。

---

## 11. 总结

ChampSim 当前已经提供：

```text
可配置乱序核心
+ 可配置缓存层次
+ 虚拟内存与 PTW
+ DRAM 模型
+ 分支预测和 BTB
+ 预取器和替换策略
+ Trace 输入
+ 文本和 JSON 统计
+ 单元测试和 Trace 工具
```

最合适的扩展方向可以概括为四条主线：

1. **实验平台主线**：批量运行、结果解析、比较和可视化。
2. **指标主线**：结构化指标、统一指标 Schema 和自定义研究指标。
3. **策略主线**：新预取器、新替换策略、新分支预测器和新 DRAM 调度器。
4. **系统模型主线**：多核 QoS、PTW、虚拟内存、能耗、可靠性和安全。

对于课程大作业，建议把主线限定为：

```text
结构化指标
  + 多预取器/多 Trace 并行运行
  + 自动比较和可视化
  + 一个新增预取研究指标
```

这样既能体现面向对象设计，又能保留硬件研究方向的实际价值。
