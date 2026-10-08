# ChampSim Trace 目录

## 已下载

### 429.mcf

```text
文件：429.mcf-217B.champsimtrace.xz
来源：https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/429.mcf-217B.champsimtrace.xz
压缩大小：220147452 bytes
SHA256：0f65748860e9f83468f0aef707f376681bdcbd9e056233ae039cb1be8ec88541
```

该文件是 DPC-3 官方 SPEC CPU Trace 集合中的 `429.mcf` 217B 版本。

容器内路径：

```text
/traces/429.mcf-217B.champsimtrace.xz
```

最小验证命令：

```bash
bin/champsim \
  --warmup-instructions 1000000 \
  --simulation-instructions 2000000 \
  --json /workspace/results/429mcf-217B-baseline.json \
  /traces/429.mcf-217B.champsimtrace.xz
```
