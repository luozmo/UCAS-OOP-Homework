# ChampSim Docker 开发环境

## 1. 构建镜像

在仓库根目录执行：

```powershell
docker compose build
```

## 2. 进入开发容器

```powershell
docker compose run --rm champsim
```

## 3. 安装 vcpkg 依赖

容器内：

```bash
vcpkg install
```

## 4. 生成配置并编译

```bash
python3 config.sh champsim_config.json
make -j"$(nproc)"
```

## 5. 运行测试

```bash
make test
make pytest
```

## 6. Trace 目录

Compose 默认把仓库中的 `traces/` 挂载到容器的 `/traces`：

```text
D:\桌面\oop\ChampSim-master\traces  ->  /traces
```

目前已下载：

```text
/traces/429.mcf-217B.champsimtrace.xz
```

快速测试：

```bash
mkdir -p /workspace/results
bin/champsim \
  --warmup-instructions 1000000 \
  --simulation-instructions 2000000 \
  --json /workspace/results/429mcf-baseline.json \
  /traces/429.mcf-217B.champsimtrace.xz
```

如果是其他外部 Trace 目录，也可以额外挂载：

```powershell
docker compose run --rm `
  -v "D:\traces:/external-traces:ro" `
  champsim
```

## 已验证环境

当前镜像已经完成以下验证：

```text
Ubuntu 22.04.5 LTS
GCC 11.4.0
CMake 3.22.1
GNU Make 4.3
Ninja 1.10.1
Python 3.10.12
vcpkg 2026-09-26
```

验证结果：

```text
python3 config.sh champsim_config.json 通过
make -j$(nproc) 通过
make -j$(nproc) test 通过
11388 assertions in 624 test cases
make pytest 通过
228 Python tests passed, 1 skipped
bin/champsim --help 通过
```

## 429.mcf 基线验证

使用 `429.mcf-217B.champsimtrace.xz` 完成快速仿真：

```text
Warmup：1000000 instructions
Simulation：2000000 instructions
ROI instructions：2000004
ROI cycles：2642543
IPC：0.756848
Wall time：15.394851859 seconds
```
