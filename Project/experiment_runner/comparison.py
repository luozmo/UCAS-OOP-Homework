"""Structured metric collection and cross-run comparison."""

from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Mapping, Optional, Tuple


@dataclass(frozen=True, order=True)
class MetricKey:
    """Unique identity of one structured metric record."""

    phase: str
    scope: str
    metric_id: str
    dimensions: Tuple[Tuple[str, str], ...] = ()

    @classmethod
    def from_record(cls, record: dict) -> "MetricKey":
        dimensions = record.get("dimensions", {})
        if not isinstance(dimensions, dict):
            raise ValueError("Metric dimensions must be a JSON object")
        normalized = tuple(sorted((str(key), str(value)) for key, value in dimensions.items()))
        return cls(
            phase=str(record["phase"]),
            scope=str(record["scope"]),
            metric_id=str(record["metric_id"]),
            dimensions=normalized,
        )

    @property
    def dimension_text(self) -> str:
        if not self.dimensions:
            return ""
        return ", ".join(f"{key}={value}" for key, value in self.dimensions)

    @property
    def label(self) -> str:
        suffix = f" [{self.dimension_text}]" if self.dimensions else ""
        return f"{self.scope}.{self.metric_id}{suffix}"


@dataclass(frozen=True)
class MetricSample:
    key: MetricKey
    value: float
    unit: str


@dataclass(frozen=True)
class CollectedRun:
    run_id: str
    status: str
    result_path: Path
    metrics_path: Path
    samples: Mapping[MetricKey, MetricSample]

    def sample(self, key: MetricKey) -> Optional[MetricSample]:
        return self.samples.get(key)


@dataclass(frozen=True)
class CollectedBatch:
    batch_id: str
    status: str
    batch_result_path: Path
    runs: Tuple[CollectedRun, ...]

    def run(self, run_id: str) -> CollectedRun:
        for run in self.runs:
            if run.run_id == run_id:
                return run
        raise KeyError(f"Unknown run_id: {run_id}")


class ResultCollector:
    """Load run artifacts and convert JSONL rows into indexed metric samples."""

    def collect_run(self, run_result_path: Path) -> CollectedRun:
        result_path = self._resolve_file(run_result_path, "run_result.json")
        payload = self._read_json(result_path)
        run_id = payload["run_id"]
        metrics_path = Path(payload["artifacts"]["metrics_jsonl"]["path"])
        if not metrics_path.is_file():
            raise FileNotFoundError(f"Metrics file not found: {metrics_path}")

        samples = {}
        with metrics_path.open("r", encoding="utf-8") as input_file:
            for line_number, line in enumerate(input_file, start=1):
                if not line.strip():
                    continue
                record = json.loads(line)
                if record.get("run_id") != run_id:
                    raise ValueError(
                        f"Metric run_id mismatch at {metrics_path}:{line_number}: "
                        f"expected {run_id}, got {record.get('run_id')}"
                    )
                key = MetricKey.from_record(record)
                if key in samples:
                    raise ValueError(f"Duplicate metric key in {metrics_path}: {key}")
                value = record.get("value")
                if isinstance(value, bool) or not isinstance(value, (int, float)):
                    raise ValueError(f"Metric value must be numeric in {metrics_path}: {key}")
                samples[key] = MetricSample(
                    key=key,
                    value=float(value),
                    unit=str(record.get("unit", "")),
                )

        if not samples:
            raise ValueError(f"No structured metrics found: {metrics_path}")
        return CollectedRun(
            run_id=run_id,
            status=str(payload.get("status", "unknown")),
            result_path=result_path,
            metrics_path=metrics_path,
            samples=samples,
        )

    def collect_batch(self, batch_result_path: Path) -> CollectedBatch:
        result_path = self._resolve_file(batch_result_path, "batch_result.json")
        payload = self._read_json(result_path)
        runs = []
        seen_run_ids = set()
        for run_summary in payload.get("runs", []):
            run_id = run_summary["run_id"]
            if run_id in seen_run_ids:
                raise ValueError(f"Duplicate run_id in batch result: {run_id}")
            seen_run_ids.add(run_id)
            run = self.collect_run(Path(run_summary["result_path"]))
            if run.run_id != run_id:
                raise ValueError(
                    f"Batch run_id mismatch: expected {run_id}, collected {run.run_id}"
                )
            runs.append(run)
        if not runs:
            raise ValueError(f"Batch contains no completed runs: {result_path}")
        return CollectedBatch(
            batch_id=str(payload["batch_id"]),
            status=str(payload.get("status", "unknown")),
            batch_result_path=result_path,
            runs=tuple(runs),
        )

    @staticmethod
    def _resolve_file(path: Path, default_name: str) -> Path:
        path = Path(path)
        return path / default_name if path.is_dir() else path

    @staticmethod
    def _read_json(path: Path) -> dict:
        with path.open("r", encoding="utf-8") as input_file:
            return json.load(input_file)


@dataclass(frozen=True)
class MetricSelector:
    scope: str
    metric_id: str
    phase: str = "roi"
    dimensions: Tuple[Tuple[str, str], ...] = ()

    def matches(self, key: MetricKey) -> bool:
        return (
            key.phase == self.phase
            and key.scope == self.scope
            and key.metric_id == self.metric_id
            and key.dimensions == self.dimensions
        )


@dataclass(frozen=True)
class MetricComparison:
    baseline: MetricSample
    candidate: MetricSample

    @property
    def key(self) -> MetricKey:
        return self.baseline.key

    @property
    def absolute_delta(self) -> float:
        return self.candidate.value - self.baseline.value

    @property
    def percent_change(self) -> Optional[float]:
        if self.baseline.value == 0:
            return None
        return (self.candidate.value - self.baseline.value) / self.baseline.value * 100.0

    def to_dict(self) -> dict:
        return {
            "phase": self.key.phase,
            "scope": self.key.scope,
            "metric_id": self.key.metric_id,
            "dimensions": dict(self.key.dimensions),
            "unit": self.baseline.unit,
            "baseline_value": self.baseline.value,
            "candidate_value": self.candidate.value,
            "absolute_delta": self.absolute_delta,
            "percent_change": self.percent_change,
        }


@dataclass(frozen=True)
class RunComparison:
    baseline_run_id: str
    candidate_run_id: str
    metrics: Tuple[MetricComparison, ...]
    missing_in_baseline: Tuple[MetricKey, ...]
    missing_in_candidate: Tuple[MetricKey, ...]

    def to_dict(self) -> dict:
        return {
            "baseline_run_id": self.baseline_run_id,
            "candidate_run_id": self.candidate_run_id,
            "common_metric_count": len(self.metrics),
            "missing_in_baseline": [key.label for key in self.missing_in_baseline],
            "missing_in_candidate": [key.label for key in self.missing_in_candidate],
            "metrics": [metric.to_dict() for metric in self.metrics],
        }


_DEFAULT_SELECTORS = (
    MetricSelector("cpu0", "ipc"),
    MetricSelector("cpu0", "cycles"),
    MetricSelector("cpu0_L2C", "cache_miss", dimensions=(("access_type", "LOAD"),)),
    MetricSelector("cpu0_L2C", "cache_hit", dimensions=(("access_type", "LOAD"),)),
    MetricSelector("cpu0_L2C", "prefetch_requested"),
    MetricSelector("cpu0_L2C", "prefetch_issued"),
    MetricSelector("cpu0_L2C", "prefetch_useful"),
    MetricSelector("cpu0_L2C", "prefetch_useless"),
    MetricSelector("cpu0_L2C", "prefetch_accuracy"),
    MetricSelector("cpu0_L2C", "prefetch_coverage"),
    MetricSelector("cpu0_L2C", "prefetch_timely_rate"),
    MetricSelector("cpu0_L2C", "prefetch_late_rate"),
    MetricSelector("cpu0_L2C", "prefetch_expiration_rate"),
    MetricSelector("cpu0_L2C", "prefetch_mean_lead_time"),
    MetricSelector("cpu0_L2C", "prefetch_timing_fills"),
    MetricSelector("cpu0_L2C", "prefetch_timing_timely"),
    MetricSelector("cpu0_L2C", "prefetch_timing_late"),
    MetricSelector("cpu0_L2C", "prefetch_timing_expired"),
    MetricSelector("cpu0_L2C", "prefetch_source_issued", dimensions=(("prefetcher_id", "0"),)),
    MetricSelector("cpu0_L2C", "prefetch_source_useful", dimensions=(("prefetcher_id", "0"),)),
    MetricSelector("cpu0_L2C", "prefetch_source_accuracy", dimensions=(("prefetcher_id", "0"),)),
    MetricSelector("cpu0_L2C", "prefetch_source_timely_rate", dimensions=(("prefetcher_id", "0"),)),
    MetricSelector("cpu0_L2C", "prefetch_source_issued", dimensions=(("prefetcher_id", "1"),)),
    MetricSelector("cpu0_L2C", "prefetch_source_useful", dimensions=(("prefetcher_id", "1"),)),
    MetricSelector("cpu0_L2C", "prefetch_source_accuracy", dimensions=(("prefetcher_id", "1"),)),
    MetricSelector("cpu0_L2C", "prefetch_source_timely_rate", dimensions=(("prefetcher_id", "1"),)),
)


@dataclass(frozen=True)
class ComparisonReport:
    baseline_run_id: str
    candidates: Tuple[RunComparison, ...]
    generated_at: str

    def to_dict(self) -> dict:
        return {
            "baseline_run_id": self.baseline_run_id,
            "generated_at": self.generated_at,
            "candidates": [candidate.to_dict() for candidate in self.candidates],
        }

    def to_markdown(self, selectors: Iterable[MetricSelector] = _DEFAULT_SELECTORS) -> str:
        lines = [
            "# ChampSim 实验结果比较",
            "",
            f"- 基线运行：`{self.baseline_run_id}`",
            f"- 生成时间：`{self.generated_at}`",
            "",
        ]

        selectors = tuple(selectors)
        for candidate in self.candidates:
            lines.extend(
                [
                    f"## `{candidate.candidate_run_id}` vs `{candidate.baseline_run_id}`",
                    "",
                    "| 指标 | 基线 | 候选 | 绝对变化 | 相对变化 |",
                    "|---|---:|---:|---:|---:|",
                ]
            )
            emitted = set()
            for selector in selectors:
                matches = [
                    metric for metric in candidate.metrics if selector.matches(metric.key)
                ]
                for metric in matches:
                    if metric.key in emitted:
                        continue
                    emitted.add(metric.key)
                    lines.append(self._markdown_row(metric))
            lines.extend(
                [
                    "",
                    f"- 共有指标：{len(candidate.metrics)}",
                    f"- 基线缺少：{len(candidate.missing_in_baseline)}",
                    f"- 候选缺少：{len(candidate.missing_in_candidate)}",
                    "",
                ]
            )
        return "\n".join(lines)

    @staticmethod
    def _markdown_row(metric: MetricComparison) -> str:
        value_mode = _is_percentage_metric(metric.key.metric_id, metric.baseline.unit)
        baseline_text = _format_value(metric.baseline.value, value_mode)
        candidate_text = _format_value(metric.candidate.value, value_mode)
        delta_text = _format_value(metric.absolute_delta, value_mode, signed=True)
        change = "N/A" if metric.percent_change is None else f"{metric.percent_change:+.3f}%"
        return f"| `{metric.key.label}` | {baseline_text} | {candidate_text} | {delta_text} | {change} |"


class ExperimentComparator:
    """Compare every candidate run against one selected baseline run."""

    def compare(self, batch: CollectedBatch, baseline_run_id: str) -> ComparisonReport:
        baseline = batch.run(baseline_run_id)
        comparisons = []
        for candidate in batch.runs:
            if candidate.run_id == baseline_run_id:
                continue
            baseline_keys = set(baseline.samples)
            candidate_keys = set(candidate.samples)
            common_keys = sorted(baseline_keys & candidate_keys)
            metric_comparisons = tuple(
                MetricComparison(
                    baseline=baseline.samples[key],
                    candidate=candidate.samples[key],
                )
                for key in common_keys
            )
            comparisons.append(
                RunComparison(
                    baseline_run_id=baseline.run_id,
                    candidate_run_id=candidate.run_id,
                    metrics=metric_comparisons,
                    missing_in_baseline=tuple(sorted(candidate_keys - baseline_keys)),
                    missing_in_candidate=tuple(sorted(baseline_keys - candidate_keys)),
                )
            )

        if not comparisons:
            raise ValueError("Comparison requires at least one non-baseline run")
        return ComparisonReport(
            baseline_run_id=baseline.run_id,
            candidates=tuple(comparisons),
            generated_at=datetime.now(timezone.utc).isoformat(),
        )


class ComparisonReportWriter:
    """Persist a comparison report as JSON and readable Markdown."""

    def __init__(self, output_dir: Path) -> None:
        self.output_dir = Path(output_dir)

    def write(self, report: ComparisonReport) -> Tuple[Path, Path]:
        self.output_dir.mkdir(parents=True, exist_ok=True)
        json_path = self.output_dir / "comparison.json"
        markdown_path = self.output_dir / "comparison.md"
        with json_path.open("w", encoding="utf-8") as output:
            json.dump(report.to_dict(), output, indent=2, ensure_ascii=False)
            output.write("\n")
        markdown_path.write_text(report.to_markdown(), encoding="utf-8")
        return json_path, markdown_path


def _is_percentage_metric(metric_id: str, unit: str) -> bool:
    return unit in {"fraction", "ratio"} or any(
        token in metric_id for token in ("rate", "accuracy", "coverage")
    )


def _format_value(value: float, percentage: bool, signed: bool = False) -> str:
    if percentage:
        return f"{value * 100.0:+.3f}%" if signed else f"{value * 100.0:.3f}%"
    if abs(value - round(value)) < 1e-9:
        return f"{int(round(value)):+,}" if signed else f"{int(round(value)):,}"
    return f"{value:+.6f}" if signed else f"{value:.6f}"
