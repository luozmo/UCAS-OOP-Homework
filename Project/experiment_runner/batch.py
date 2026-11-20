"""Parallel orchestration for a batch of independent ChampSim runs."""

from __future__ import annotations

import json
import re
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from datetime import datetime, timezone
from enum import Enum
from pathlib import Path
from typing import Optional, Tuple

from .execution import ExecutionBackend
from .runner import ExperimentRunner, RunResult, RunStatus
from .spec import ExperimentSpec


_RUN_ID_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")


class BatchStatus(str, Enum):
    """Aggregate state for all runs in a batch."""

    SUCCEEDED = "succeeded"
    PARTIAL = "partial"
    FAILED = "failed"


@dataclass(frozen=True)
class BatchTaskError:
    """Execution error that prevented a run from producing a RunResult."""

    run_id: str
    error_type: str
    message: str

    def to_dict(self) -> dict:
        return {
            "run_id": self.run_id,
            "error_type": self.error_type,
            "message": self.message,
        }


@dataclass(frozen=True)
class ExperimentBatchSpec:
    """Immutable description of several isolated runs.

    The batch directory contains only orchestration metadata. Each child run
    still owns its own directory and complete RunResult.
    """

    batch_id: str
    output_dir: Path
    runs: Tuple[ExperimentSpec, ...]
    max_workers: int = 1

    def __post_init__(self) -> None:
        if not _RUN_ID_PATTERN.fullmatch(self.batch_id):
            raise ValueError(
                "batch_id must start with a letter or digit and contain only "
                "letters, digits, '.', '_' or '-'"
            )
        object.__setattr__(self, "output_dir", Path(self.output_dir))
        object.__setattr__(self, "runs", tuple(self.runs))
        if not self.runs:
            raise ValueError("A batch must contain at least one run")
        if self.max_workers <= 0:
            raise ValueError("max_workers must be positive")

        run_ids = [run.run_id for run in self.runs]
        if len(run_ids) != len(set(run_ids)):
            raise ValueError("run_id values must be unique within a batch")

        output_dirs = [Path(run.output_dir) for run in self.runs]
        if len(output_dirs) != len(set(output_dirs)):
            raise ValueError("run output directories must be unique within a batch")

    @property
    def batch_spec_path(self) -> Path:
        return self.output_dir / "batch_spec.json"

    @property
    def batch_result_path(self) -> Path:
        return self.output_dir / "batch_result.json"

    def resolve(self, base_dir: Path) -> "ExperimentBatchSpec":
        base_dir = Path(base_dir)
        resolved_output_dir = self.output_dir if self.output_dir.is_absolute() else base_dir / self.output_dir
        resolved_runs = tuple(run.resolve(base_dir) for run in self.runs)
        resolved = ExperimentBatchSpec(
            batch_id=self.batch_id,
            output_dir=resolved_output_dir,
            runs=resolved_runs,
            max_workers=self.max_workers,
        )
        resolved._validate_run_directories()
        return resolved

    def to_dict(self) -> dict:
        return {
            "batch_id": self.batch_id,
            "output_dir": str(self.output_dir),
            "max_workers": self.max_workers,
            "runs": [run.to_dict() for run in self.runs],
        }

    def _validate_run_directories(self) -> None:
        root = self.output_dir.resolve()
        for run in self.runs:
            child = run.output_dir.resolve()
            try:
                relative = child.relative_to(root)
            except ValueError as error:
                raise ValueError(
                    f"Run output directory {child} must be inside batch directory {root}"
                ) from error
            if relative == Path("."):
                raise ValueError("A run output directory cannot equal the batch directory")


@dataclass(frozen=True)
class ExperimentBatchResult:
    """Serializable result for one completed batch."""

    spec: ExperimentBatchSpec
    status: BatchStatus
    results: Tuple[RunResult, ...]
    errors: Tuple[BatchTaskError, ...]
    elapsed_seconds: float
    started_at: str
    finished_at: str

    @property
    def succeeded_count(self) -> int:
        return sum(result.succeeded for result in self.results)

    @property
    def failed_count(self) -> int:
        return len(self.spec.runs) - self.succeeded_count

    def to_dict(self) -> dict:
        run_summaries = []
        for result in self.results:
            run_summaries.append(
                {
                    "run_id": result.spec.run_id,
                    "status": result.status.value,
                    "return_code": result.return_code,
                    "timed_out": result.timed_out,
                    "elapsed_seconds": result.elapsed_seconds,
                    "output_dir": str(result.spec.output_dir),
                    "result_path": str(result.spec.result_path),
                    "metrics_jsonl": str(result.spec.metrics_jsonl_path),
                }
            )

        return {
            "batch_id": self.spec.batch_id,
            "status": self.status.value,
            "elapsed_seconds": self.elapsed_seconds,
            "started_at": self.started_at,
            "finished_at": self.finished_at,
            "run_count": len(self.spec.runs),
            "succeeded_count": self.succeeded_count,
            "failed_count": self.failed_count,
            "max_workers": self.spec.max_workers,
            "runs": run_summaries,
            "errors": [error.to_dict() for error in self.errors],
            "spec": self.spec.to_dict(),
        }


class ExperimentBatchRunner:
    """Run several isolated ExperimentSpec objects concurrently.

    Threads are used only to wait for independent subprocesses. ChampSim itself
    remains single-threaded per run, so microarchitectural state is never shared.
    """

    def __init__(
        self,
        working_directory: Path,
        execution_backend: Optional[ExecutionBackend] = None,
    ) -> None:
        self.working_directory = Path(working_directory).resolve()
        self.execution_backend = execution_backend

    def run(self, batch: ExperimentBatchSpec) -> ExperimentBatchResult:
        resolved = batch.resolve(self.working_directory)
        resolved.output_dir.mkdir(parents=True, exist_ok=False)
        self._write_json(resolved.batch_spec_path, resolved.to_dict())

        started_at = datetime.now(timezone.utc).isoformat()
        started_clock = time.perf_counter()

        runner = ExperimentRunner(
            working_directory=self.working_directory,
            execution_backend=self.execution_backend,
        )
        results_by_run_id = {}
        errors = []

        # Submitting every run first allows Docker/ChampSim processes to overlap.
        # Future objects are then consumed in input order for deterministic output.
        with ThreadPoolExecutor(max_workers=resolved.max_workers) as executor:
            futures = {
                run.run_id: executor.submit(runner.run, run)
                for run in resolved.runs
            }
            for run in resolved.runs:
                try:
                    results_by_run_id[run.run_id] = futures[run.run_id].result()
                except Exception as error:  # Keep other completed runs and record task failures.
                    errors.append(
                        BatchTaskError(
                            run_id=run.run_id,
                            error_type=type(error).__name__,
                            message=str(error),
                        )
                    )

        elapsed_seconds = time.perf_counter() - started_clock
        finished_at = datetime.now(timezone.utc).isoformat()
        ordered_results = tuple(
            results_by_run_id[run.run_id]
            for run in resolved.runs
            if run.run_id in results_by_run_id
        )
        successful_count = sum(result.status == RunStatus.SUCCEEDED for result in ordered_results)

        if not errors and successful_count == len(resolved.runs):
            status = BatchStatus.SUCCEEDED
        elif successful_count > 0:
            status = BatchStatus.PARTIAL
        else:
            status = BatchStatus.FAILED

        result = ExperimentBatchResult(
            spec=resolved,
            status=status,
            results=ordered_results,
            errors=tuple(errors),
            elapsed_seconds=elapsed_seconds,
            started_at=started_at,
            finished_at=finished_at,
        )
        self._write_json(resolved.batch_result_path, result.to_dict())
        return result

    @staticmethod
    def _write_json(path: Path, value: dict) -> None:
        with path.open("w", encoding="utf-8") as output:
            json.dump(value, output, indent=2, ensure_ascii=False)
            output.write("\n")
