# ChampSim 代码层次结构分析

> 分析对象：当前工作区中的 ChampSim 源码。本文从构建配置、运行时对象图、逐周期调度、处理器核心、存储层次、Trace 输入、策略模块和统计输出八个方面梳理代码层次与依赖关系。

---

## 1. 项目定位

ChampSim 是一个基于 Trace 的微架构模拟器，主要模拟：

- 乱序处理器前端和乱序执行流水线；
- 一级、二级和末级缓存；
- 分支预测器和分支目标缓冲；
- 数据预取器和缓存替换策略；
- 页表遍历与虚拟地址翻译；
- DRAM 地址映射、Bank 调度、刷新和读写总线。

它采用“编译期配置 + 运行时逐周期仿真”的结构。用户先通过 JSON 配置生成具体 C++ 环境，再编译一个与该配置绑定的模拟器可执行文件。

---

## 2. 目录结构与职责

| 目录 | 主要内容 | 所在层次 |
| --- | --- | --- |
| `config/` | JSON 解析、默认值、模块搜索、代码生成、Makefile 生成 | 构建与配置层 |
| `config.sh` | 配置生成入口 | 构建与配置层 |
| `src/` | 主入口、仿真主循环、核心、缓存、DRAM、PTW 等实现 | 运行时核心层 |
| `inc/` | 核心类、接口、工具、统计结构和公共类型 | 运行时核心层 |
| `branch/` | 分支方向预测器 | 策略模块层 |
| `btb/` | 分支目标预测器 | 策略模块层 |
| `prefetcher/` | 缓存预取器 | 策略模块层 |
| `replacement/` | 缓存替换策略 | 策略模块层 |
| `test/` | C++ 单元测试、Python 配置测试和配置样例 | 测试与验证层 |
| `tracer/` | Pin Trace 生成工具和 CVP Trace 转换工具 | 外部工具层 |
| `docs/` | 模块接口、配置 API 和模型文档 | 文档层 |
| `vcpkg.json` | 第三方依赖声明 | 构建依赖层 |

源码目录主要按“运行时角色和领域对象”划分，而不是严格按 Controller、Service、Repository 等 Web 应用分层命名。

---

## 3. 总体分层结构

```text
+---------------------------------------------------------------+
| 用户与外部工具层                                                |
| champsim_config.json / Trace 文件 / CLI / tracer/              |
+---------------------------------------------------------------+
                              |
                              v
+---------------------------------------------------------------+
| 构建与配置生成层                                                |
| config.sh -> config/parse.py -> defaults/modules ->             |
| config/instantiation_file.py -> core_inst.inc / Makefile        |
+---------------------------------------------------------------+
                              |
                              v
+---------------------------------------------------------------+
| 模拟器入口与运行时编排层                                        |
| src/main.cc -> generated_environment -> champsim::main          |
| src/champsim.cc -> do_phase / do_cycle                          |
+---------------------------------------------------------------+
                              |
                              v
+---------------------------------------------------------------+
| 逐周期仿真对象层                                                |
| champsim::operable                                              |
|    +-- O3_CPU                                                    |
|    +-- CACHE                                                     |
|    +-- PageTableWalker                                           |
|    +-- MEMORY_CONTROLLER                                         |
|    +-- DRAM_CHANNEL                                              |
+---------------------------------------------------------------+
                              |
          +-------------------+-------------------+
          |                                       |
          v                                       v
+---------------------------+       +-------------------------------+
| 处理器核心与前端层         |       | 存储层次与地址翻译层           |
| O3_CPU                    |       | CACHE / channel / MSHR         |
| RegisterAllocator         |       | PageTableWalker / VirtualMemory|
| branch predictor / BTB    |       | MEMORY_CONTROLLER / DRAM       |
+---------------------------+       +-------------------------------+
          |                                       |
          +-------------------+-------------------+
                              |
                              v
+---------------------------------------------------------------+
| Trace 输入与指令模型层                                          |
| tracereader -> bulk_tracereader -> trace_instruction            |
| -> ooo_model_instr                                             |
+---------------------------------------------------------------+
                              |
                              v
+---------------------------------------------------------------+
| 统计、阶段与输出层                                              |
| cpu_stats / cache_stats / dram_stats -> phase_stats             |
| plain_printer / json_printer -> stdout / JSON                   |
+---------------------------------------------------------------+
                              |
                              v
+---------------------------------------------------------------+
| 基础类型和工具层                                                |
| address_slice / extent / units / bandwidth / chrono / MSL       |
+---------------------------------------------------------------+
```

---

## 4. 构建与配置生成层

### 4.1 入口

`config.sh` 是配置生成入口，负责：

1. 读取一个或多个 JSON 配置文件；
2. 根据 `--join chain/product` 组合多个配置；
3. 调用 `config.parse.parse_config()`；
4. 通过 `config.filewrite.FileWriter` 生成构建文件；
5. 为不同配置生成不同的可执行文件构建目标。

### 4.2 配置对象

`config/parse.py` 中的 `NormalizedConfiguration` 是配置标准化对象，主要职责是：

- 展开 CPU、缓存、PTW、物理内存和虚拟内存配置；
- 推断 CPU 对应的 L1I、L1D、ITLB、DTLB、L2C、STLB、PTW 和 LLC；
- 自动连接上层和下层的缓存路径；
- 填充默认大小、队列、延迟和频率；
- 解析模块目录；
- 准备替换策略和预取器描述。

### 4.3 配置转换成对象图

`config/instantiation_file.py` 将标准化配置转换成 C++ 构造代码：

```text
JSON 配置
  -> channels 数组
  -> MEMORY_CONTROLLER
  -> VirtualMemory
  -> PageTableWalker 数组
  -> CACHE 数组
  -> O3_CPU 数组
  -> generated_environment
```

生成结果包括：

- `core_inst.inc`：`generated_environment` 类声明；
- `core_inst.cc.inc`：具体对象构造和视图函数；
- `_configuration.mk`：配置绑定的编译依赖与构建变量。

### 4.4 依赖方向

```text
JSON
  -> NormalizedConfiguration
  -> parse_config
  -> get_instantiation_lines / get_instantiation_header
  -> FileWriter
  -> C++ 生成代码 + Makefile 片段
```

配置生成层不参与运行时仿真，但它决定了运行时对象数量和连接拓扑。

---

## 5. 入口与运行时编排层

### 5.1 `src/main.cc`

`src/main.cc` 是真实程序入口，负责：

- 构造 `generated_environment`；
- 解析命令行参数；
- 创建 `tracereader`；
- 创建 Warmup 和 Simulation 两个 `phase_info`；
- 调用 `champsim::main()`；
- 输出文本统计或 JSON 统计。

`main.cc` 不负责具体处理器或缓存行为，只负责启动和最终输出。

### 5.2 `champsim::environment`

`inc/environment.h` 定义运行时对象集合的统一访问接口：

```cpp
cpu_view()
cache_view()
ptw_view()
dram_view()
operable_view()
```

`generated_environment` 是具体实现，内部持有：

```text
channels
DRAM
vmem
ptws
caches
cores
```

### 5.3 `src/champsim.cc`

该文件是仿真时间推进和阶段管理的核心：

- `do_cycle()`：推进一个最小时间步；
- `do_phase()`：运行 Warmup 或 Simulation 阶段；
- `champsim::main()`：初始化组件并按阶段运行。

逐周期流程：

```text
global_clock.tick(time_quantum)
  -> env.operable_view()
  -> 按 current_time 排序
  -> 对每个 operable 调用 operate_on(global_clock)
  -> 从 trace 补满 CPU input_queue
  -> 检查死锁、Trace EOF 和阶段完成
```

### 5.4 `champsim::operable`

`inc/operable.h` 和 `src/operable.cc` 定义所有逐周期组件的共同生命周期：

```text
initialize()
operate_on(clock)
  -> _operate()
     -> current_time += clock_period
     -> operate()
begin_phase()
end_phase(cpu)
print_deadlock()
```

不同组件可以有不同 `clock_period`。`operate_on()` 会反复调用 `_operate()`，直到组件本地时间追上全局时钟。

---

## 6. 运行时对象图

简化后的默认单核对象图如下：

```text
generated_environment
  |
  +-- channels: vector<champsim::channel>
  +-- DRAM: MEMORY_CONTROLLER
  |     +-- channels: vector<DRAM_CHANNEL>
  +-- vmem: VirtualMemory
  +-- ptws: vector<PageTableWalker>
  +-- caches: vector<CACHE>
  +-- cores: vector<O3_CPU>

O3_CPU
  +-- L1I_bus -> fetch channel -> L1I CACHE
  +-- L1D_bus -> data channel  -> L1D CACHE
  +-- branch predictor
  +-- BTB

L1I / L1D
  +-- lower_translate -> DTLB / ITLB / STLB
  +-- lower_level -> L2C
  +-- upper_levels -> channel queues

L2C
  +-- lower_level -> LLC
  +-- lower_translate -> STLB / PTW path

LLC
  +-- lower_level -> DRAM request channel

PageTableWalker
  +-- upper_levels -> TLB miss channels
  +-- lower_level -> cache/DRAM request channel
  +-- vmem: VirtualMemory
```

对象的不变量：

- `channel` 只负责队列、请求、响应和队列统计，不负责缓存命中判断。
- `CACHE` 负责标签、MSHR、填充、写回、预取器和替换策略。
- `O3_CPU` 负责指令级流水线和 L1 访问发起。
- `PageTableWalker` 负责多级页表遍历状态。
- `VirtualMemory` 负责虚拟页和物理页映射。
- `MEMORY_CONTROLLER` 负责把请求分发到 DRAM 通道。
- `DRAM_CHANNEL` 负责 Bank、Row、Column、刷新和数据总线调度。

---

## 7. 处理器核心层

### 7.1 `O3_CPU`

`O3_CPU` 继承 `champsim::operable`，是处理器核心的主要对象。

主要状态结构：

| 成员 | 作用 |
| --- | --- |
| `IFETCH_BUFFER` | 取指缓冲区 |
| `DECODE_BUFFER` | 译码缓冲区 |
| `DISPATCH_BUFFER` | 分派缓冲区 |
| `ROB` | 重排序缓冲区 |
| `LQ` / `SQ` | Load Queue 和 Store Queue |
| `DIB` | 已译码指令缓冲区 |
| `input_queue` | 从 Trace 注入的指令 |
| `RegisterAllocator` | 物理寄存器分配和重命名 |
| `CacheBus L1I_bus` | 访问 L1I 的请求接口 |
| `CacheBus L1D_bus` | 访问 L1D 的请求接口 |

### 7.2 逐周期流水线顺序

`O3_CPU::operate()` 的子步骤顺序为：

```text
retire_rob
complete_inflight_instruction
execute_instruction
schedule_instruction
handle_memory_return
operate_lsq
dispatch_instruction
decode_instruction
promote_to_decode
fetch_instruction
check_dib
initialize_instruction
```

该顺序体现了反向传播式的流水线更新：先处理退休和完成，再处理执行、调度、访存、分派、译码和取指。

### 7.3 核心相关对象

| 对象 | 职责 |
| --- | --- |
| `ooo_model_instr` | 核心内部指令模型 |
| `LSQ_ENTRY` | Load/Store Queue 条目 |
| `RegisterAllocator` | 寄存器重命名、释放和依赖维护 |
| `CacheBus` | CPU 到 L1 缓存通道的适配器 |
| 分支预测器模块 | 方向预测 |
| BTB 模块 | 目标地址预测 |

---

## 8. 存储层次层

### 8.1 `champsim::channel`

`channel` 是组件之间的数据通道，内部保存：

- `RQ`：Read Queue；
- `WQ`：Write Queue；
- `PQ`：Prefetch Queue；
- `returned`：返回给上层的响应队列；
- `sim_stats` 和 `roi_stats`：队列统计。

队列不是逐周期模拟对象，而是由缓存、PTW 和 DRAM 在各自 `operate()` 中消费。

### 8.2 `CACHE`

`CACHE` 继承 `champsim::operable`，主要职责包括：

- 地址到 Set 的映射；
- Tag 查找和 Way 选择；
- MSHR 合并；
- Fill、Eviction 和 Writeback；
- 物理地址和虚拟地址处理；
- 预取请求进入和预取命中判定；
- 替换策略调用；
- 队列占用与命中/缺失统计。

`CACHE::operate()` 的主要阶段：

```text
处理下层返回
  -> 处理地址翻译返回
  -> 执行 Fill
  -> 从内部 PQ、上层 WQ/RQ/PQ 发起 Tag Check
  -> 处理 Translation Stash
  -> 执行 Hit / Miss / Write
  -> 调用预取器逐周期回调
```

### 8.3 `PageTableWalker`

`PageTableWalker` 继承 `champsim::operable`，维护：

- 上层 TLB Miss 请求；
- 多层页表遍历状态；
- `MSHR`；
- `finished` 和 `completed` 队列；
- `pscl` 页表缓存结构；
- 指向 `VirtualMemory` 的引用。

其逐周期流程包括：

```text
接收下层返回
  -> 推进页表查询
  -> 接收上层请求
  -> 生成下一级翻译请求
  -> 完成翻译并返回响应
```

### 8.4 `VirtualMemory`

`VirtualMemory` 不是 `operable` 对象，而是地址翻译数据模型：

- 虚拟页到物理页映射；
- 页表层级和 PTE 页面分配；
- 物理页空闲列表；
- 随机化和页面初始化；
- `va_to_pa()` 和 `get_pte_pa()`。

它被 `PageTableWalker` 引用，并通过 `MEMORY_CONTROLLER` 获取物理内存大小信息。

### 8.5 `MEMORY_CONTROLLER` 和 `DRAM_CHANNEL`

`MEMORY_CONTROLLER` 是 DRAM 的对外对象，负责：

- 接收 LLC 的请求；
- 根据地址映射选择 DRAM 通道；
- 管理和推进多个 `DRAM_CHANNEL`。

`DRAM_CHANNEL` 负责：

- RQ/WQ；
- Bank 和 Bank Group 调度；
- Row Buffer Hit/Miss；
- 刷新；
- 读写模式切换；
- 数据总线占用；
- 请求响应返回。

简化关系：

```text
LLC -> DRAM 请求 channel
    -> MEMORY_CONTROLLER
    -> DRAM_CHANNEL[0..n]
    -> Bank / Bank Group / Row / Column
```

---

## 9. Trace 输入与指令模型层

### 9.1 Trace 读取

`inc/tracereader.h` 中的 `tracereader` 使用类型擦除：

```text
tracereader
  +-- reader_concept
        +-- reader_model<T>
              +-- bulk_tracereader<T, F>
```

`bulk_tracereader` 的职责：

1. 从普通文件或压缩流批量读取 Trace Record；
2. 将二进制 Trace Record 转换为 `ooo_model_instr`；
3. 进行指令缓冲；
4. 计算分支目标；
5. 通过 `operator()` 逐条提供给 CPU。

### 9.2 指令模型

`ooo_model_instr` 包含：

- IP、ASID、分支信息；
- 调度和完成状态；
- 源寄存器、目的寄存器；
- 源内存、目的内存；
- 寄存器依赖关系；
- 指令唯一 ID。

`cpu_stats` 和 `event_counter` 以分支类型等键统计指令行为。

---

## 10. 策略模块层

ChampSim 的策略模块主要包括：

| 模块类型 | 目录 | 接口 |
| --- | --- | --- |
| 分支方向预测 | `branch/` | `champsim::modules::branch_predictor` |
| 分支目标预测 | `btb/` | `champsim::modules::btb` |
| 数据预取 | `prefetcher/` | `champsim::modules::prefetcher` |
| 缓存替换 | `replacement/` | `champsim::modules::replacement` |

模块采用编译期选择：

```text
JSON 中的 "prefetcher": "next_line"
  -> config/modules.py 搜索源码目录
  -> instantiation_file.py 生成 prefetcher<class next_line>()
  -> CACHE::prefetcher_module_model<next_line> 连接接口
  -> 编译到具体可执行文件
```

`CACHE` 内部通过 `prefetcher_module_concept`、`replacement_module_concept` 做运行时统一接口，再由模板 `prefetcher_module_model<Ps...>` 和 `replacement_module_model<Rs...>` 调用具体模块。

设计特点：

- 对外提供统一的策略钩子；
- 对内部允许使用强类型地址或旧式整数参数；
- 支持多个预取器或多个替换策略的组合；
- 模块构造时绑定到 `CACHE` 或 `O3_CPU`。

---

## 11. 统计与输出层

### 11.1 统计对象

| 对象 | 主要统计 |
| --- | --- |
| `cpu_stats` | 指令数、周期数、分支类型、误预测 |
| `cache_stats` | 命中、缺失、MSHR 合并、Fill、预取 |
| `cache_queue_stats` | RQ/WQ/PQ 访问、满队列和转发 |
| `dram_stats` | Row Buffer、刷新、Bus 拥塞 |
| `phase_stats` | 一个阶段的 CPU、CACHE、DRAM 汇总 |

每个阶段区分：

- `sim_stats`：整个仿真阶段统计；
- `roi_stats`：Region of Interest 统计。

### 11.2 输出对象

| 对象 | 职责 |
| --- | --- |
| `plain_printer` | 输出人类可读文本 |
| `json_printer` | 输出结构化 JSON |
| `phase_info` | 描述阶段名称、长度和 Trace |
| `event_listeners` | 监听 Heartbeat 等事件 |
| `Heartbeat` | 周期性打印进度 |

当前输出结构存在一个明显扩展点：统计结构是具体 C++ 类型，`json_printer` 通过重载 `to_json` 写入固定字段。如果增加新的研究指标，需要在统计对象、采集点和序列化对象之间建立更通用的接口。

---

## 12. 基础类型与工具层

### 12.1 地址类型

`inc/address.h` 提供强类型地址切片：

- `champsim::address`；
- `page_number`；
- `page_offset`；
- `block_number`；
- `block_offset`；
- `dynamic_extent` 和 `static_extent`；
- `splice()`、`offset()`、`uoffset()`。

这些类型用编译期检查降低直接操作 `uint64_t` 带来的地址错误。

### 12.2 单位和时间

| 文件 | 职责 |
| --- | --- |
| `inc/util/units.h` | 字节、页、位宽等单位 |
| `inc/chrono.h` | 模拟时钟和时间点 |
| `inc/bandwidth.h` | 每周期资源消耗模型 |
| `inc/waitable.h` | 带就绪时间的异步值 |

### 12.3 数据结构与算法

| 对象 | 用途 |
| --- | --- |
| `event_counter<K>` | 按类型键统计事件 |
| `lru_table` | DIB、PTW 缓存等 LRU 表 |
| `msl/lru_table` | 策略模块使用的 LRU 表 |
| `msl/fwcounter` | 饱和计数器 |
| `extent_set` | 地址位段和 DRAM 字段组合 |
| `util/*` | bits、ratio、span、type traits 和算法 |

---

## 13. 测试与外部工具层

### 13.1 C++ 测试

`test/cpp/src/` 按编号划分：

| 编号 | 覆盖范围 |
| --- | --- |
| `000-099` | 顶层和工具 |
| `100-199` | 核心前端 |
| `200-299` | 乱序执行核心 |
| `300-399` | 退休路径 |
| `400-499` | 缓存、预取和替换 |
| `500-599` | 环境与集成相关测试，当前包含 `500-environment.cc` |
| `600-699` | 页表遍历 |
| `700-799` | DRAM |
| `800-899` | 虚拟内存 |
| `900-999` | 特殊问题和边界情况 |

测试中使用 `mocks.hpp` 构造模拟的请求消费者、生产者和内存组件，避免所有测试都依赖完整 ChampSim 环境。

### 13.2 Python 测试

`test/python/` 主要测试：

- 配置解析；
- 默认值推断；
- 文件生成；
- 实例化代码生成；
- 路径和工具函数。

### 13.3 Trace 工具

`tracer/` 包含：

- Pin-based Trace 生成工具；
- CVP 格式到 ChampSim 格式的转换工具。

---

## 14. 运行时调用链

完整调用链可以简化表示为：

```text
config.sh
  -> parse_config
  -> generated_environment
  -> main
  -> champsim::main
  -> do_phase
  -> do_cycle
  -> operable::operate_on
  -> O3_CPU / CACHE / PTW / MEMORY_CONTROLLER::operate
  -> channel / MSHR / DRAM Channel
  -> stats
  -> phase_stats
  -> plain_printer / json_printer
```

处理器侧和存储侧的主路径：

```text
Trace
  -> O3_CPU::input_queue
  -> IFETCH_BUFFER
  -> DECODE_BUFFER
  -> DISPATCH_BUFFER
  -> ROB
  -> LQ / SQ
  -> L1D CacheBus
  -> channel RQ/WQ/PQ
  -> CACHE
  -> L2C
  -> LLC
  -> MEMORY_CONTROLLER
  -> DRAM_CHANNEL
  -> returned channel
  -> CPU 重新调度和完成
```

地址翻译路径：

```text
O3_CPU 访存
  -> L1D / L1I
  -> lower_translate channel
  -> DTLB / ITLB
  -> STLB
  -> PageTableWalker
  -> VirtualMemory
  -> 物理地址
```

---

## 15. 依赖方向与边界

### 15.1 主要依赖方向

```text
基础类型层
  <- 核心、缓存、PTW、DRAM、模块

仿真组件层
  <- environment 和 generated_environment

environment 接口
  <- src/champsim.cc 的阶段与调度逻辑

统计结构
  <- 各仿真组件

phase_stats
  <- champsim.cc

printer
  <- phase_stats
```

### 15.2 层次边界判断

| 边界 | 当前设计 |
| --- | --- |
| 配置层与运行时 | 通过生成 C++ 对象图连接 |
| CPU 与缓存 | 通过 `CacheBus` 和 `channel` 连接 |
| 缓存与缓存 | 通过 `channel` 连接 |
| 缓存与 DRAM | 通过 LLC 下层 channel 和 `MEMORY_CONTROLLER` 连接 |
| PTW 与虚拟内存 | 通过 `VirtualMemory*` 连接 |
| 策略模块与组件 | 通过模块基类、模板适配器和绑定指针连接 |
| 仿真核心与输出 | 通过 `phase_stats` 和 printer 连接 |

---

## 16. 代码中的设计特征

### 16.1 面向对象特征

- `operable` 定义统一生命周期抽象接口。
- `CACHE`、`O3_CPU`、`PageTableWalker` 和 DRAM 对象覆写 `operate()`。
- `environment` 提供统一对象视图接口。
- `core_builder`、`cache_builder` 和 `ptw_builder` 负责构造复杂对象。
- 分支预测器、BTB、预取器和替换策略采用策略式扩展。
- `tracereader` 使用 Pimpl / 类型擦除隐藏具体读取器。
- `event_counter` 封装按类型统计行为。

### 16.2 数据驱动和编译期优化

ChampSim 并非所有地方都使用经典运行时多态：

- 地址使用强类型模板。
- 策略模块在编译期实例化。
- 对象图由 Python 生成 C++ 代码。
- 缓存 Set/Way 和队列使用连续容器。
- `channel` 是数据中心数据通道，而不是纯领域对象。

这说明 ChampSim 是在“面向对象的组件边界”和“高性能数据布局”之间取得平衡。

---

## 17. 关键观察与扩展点

### 17.1 构建配置可扩展

可以增加新的：

- 缓存层级；
- 多核和异构核配置；
- 预取器；
- 替换策略；
- 分支预测器；
- PTW 参数；
- DRAM 地址映射参数。

### 17.2 统计输出相对内向

当前统计字段与 C++ 统计结构耦合较强：

- `cache_stats` 是具体结构；
- `json_printer` 通过 `to_json` 写固定字段；
- 模块自定义统计可能直接打印；
- 缺少统一 `MetricRecord` 或 `MetricSink`；
- 缺少面向多次实验的 `Experiment` / `RunResult` 对象。

这正好为“实验结果采集、并行编排与可视化子系统”提供了自然的扩展位置。

### 17.3 逐周期调度是全局核心

`src/champsim.cc` 的 `do_cycle()` 和 `champsim::operable::operate_on()` 是所有硬件组件行为的时间基准。任何实验平台都不应该直接改变这一层的行为，除非目标就是优化模拟内核。

### 17.4 对象关系由配置生成

运行时的缓存层级不是写死的类继承树，而是由 Python 配置生成的：

```text
JSON 配置 -> channel 拓扑 -> CACHE/PTW/CPU 构造 -> generated_environment
```

因此，做复杂多预取器、多 Trace 或新缓存层级实验时，应该优先修改配置生成层，而不是手工改生成文件。

---

## 18. 面向可视化与实验平台的分层建议

如果后续要增加结果采集、并行实验和图形化比较，建议把系统分成：

```text
现有 ChampSim 仿真核心
  |
  | JSON/JSONL 或结构化统计输出
  v
统计采集与结果模型层
  |
  +-- RunResult
  +-- MetricRecord
  +-- ResultSet
  |
  v
实验编排层
  |
  +-- ExperimentSpec
  +-- JobPlanner
  +-- ParallelRunScheduler
  +-- RunExecutor
  |
  v
分析对比层
  |
  +-- MetricRegistry
  +-- Aggregator
  +-- Comparator
  |
  v
可视化与报告层
  |
  +-- ChartSpec
  +-- Renderer
  +-- ReportBuilder
```

这样不会把实验编排和绘图逻辑侵入 `O3_CPU`、`CACHE` 或 `DRAM_CHANNEL`，同时仍然可以在缓存和预取器的关键事件处增加结构化埋点。

---

## 19. 总结

ChampSim 可以概括为以下层次：

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

其核心设计特点是：

1. 配置驱动对象图；
2. 统一 `operable` 时钟协议；
3. CPU、CACHE、PTW、DRAM 通过 `channel` 解耦连接；
4. 分支、BTB、预取和替换策略以模块方式扩展；
5. 统计结构与仿真组件紧密耦合；
6. 编译期配置与运行时逐周期仿真相结合；
7. 已有较完整的 C++ 单元测试和 Python 配置测试。

对于后续课程项目，最自然的两条路径是：

- **仿真核心方向**：优化 `do_cycle()`、调度结构、缓存热路径或数据复制；
- **实验平台方向**：保持仿真核心语义不变，新增结构化统计模型、并行实验管理和可视化比较对象。

