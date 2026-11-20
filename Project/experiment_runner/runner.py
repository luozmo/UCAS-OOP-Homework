"""Execution of one isolated ChampSim experiment."""

from __future__ import annotations

import json
import shutil
import subprocess
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from enum import Enum
from pathlib import Path
from typing import Optional, Tuple

from .execution import ExecutionBackend, NativeExecutionBackend
from .spec import ExperimentSpec


class RunStatus(str, Enum):
    """Final state of one experiment process."""

    SUCCEEDED = "succeeded"
    FAILED = "failed"
    TIMED_OUT = "timed_out"
    INVALID_OUTPUT = "invalid_output"


@dataclass(frozen=True)
class RunResult:
    """Serializable outcome of one ExperimentRunner.run() call."""

    spec: ExperimentSpec
    status: RunStatus
    return_code: Optional[int]
    timed_out: bool
    elapsed_seconds: float
    command: Tuple[str, ...]
    started_at: str
    finished_at: str
    error_message: Optional[str] = None

    @property
    def succeeded(self) -> bool:
        return self.status == RunStatus.SUCCEEDED

    def _artifact_size(self, path: Path) -> Optional[int]:
        return path.stat().st_size if path.exists() else None

    def to_dict(self) -> dict:
        return {
            "run_id": self.spec.run_id,
            "status": self.status.value,
            "return_code": self.return_code,
            "timed_out": self.timed_out,
            "elapsed_seconds": self.elapsed_seconds,
            "started_at": self.started_at,
            "finished_at": self.finished_at,
            "command": list(self.command),
            "error_message": self.error_message,
            "spec": self.spec.to_dict(),
            "artifacts": {
                "metrics_jsonl": {
                    "path": str(self.spec.metrics_jsonl_path),
                    "size_bytes": self._artifact_size(self.spec.metrics_jsonl_path),
                },
                "metric_definitions": {
                    "path": str(self.spec.metric_definitions_path),
                    "size_bytes": self._artifact_size(self.spec.metric_definitions_path),
                },
                "stdout": str(self.spec.stdout_path),
                "stderr": str(self.spec.stderr_path),
                "run_spec": str(self.spec.run_spec_path),
                "run_result": str(self.spec.result_path),
            },
        }


class ExperimentRunner:
    """Launch one ExperimentSpec and preserve all run artifacts.

    This first implementation is intentionally serial.  Parallel orchestration
    will compose multiple runners instead of changing the behavior of one run.
    """

    def __init__(
        self,
        working_directory: Path,
        execution_backend: Optional[ExecutionBackend] = None,
    ) -> None:
        self.working_directory = Path(working_directory).resolve()
        self.execution_backend = execution_backend or NativeExecutionBackend()

    def run(self, spec: ExperimentSpec) -> RunResult:
        resolved = spec.resolve(self.working_directory)
        self._validate_inputs(resolved)

        # exist_ok=False prevents a new experiment from silently overwriting
        # an earlier run with the same run_id/directory.
        resolved.output_dir.mkdir(parents=True, exist_ok=False)

        if resolved.config_path is not None:
            shutil.copyfile(resolved.config_path, resolved.copied_config_path)

        self._write_json(resolved.run_spec_path, resolved.to_dict())

        command = self.execution_backend.command(resolved, self.working_directory)
        started_at = datetime.now(timezone.utc).isoformat()
        started_clock = time.perf_counter()

        return_code: Optional[int] = None
        timed_out = False
        error_message: Optional[str] = None

        try:
            with resolved.stdout_path.open("w", encoding="utf-8", errors="replace") as stdout_file, \
                    resolved.stderr_path.open("w", encoding="utf-8", errors="replace") as stderr_file:
                completed = subprocess.run(
                    command,
                    cwd=self.working_directory,
                    stdout=stdout_file,
                    stderr=stderr_file,
                    timeout=resolved.timeout_seconds,
                    check=False,
                )
                return_code = completed.returncode
        except subprocess.TimeoutExpired as error:
            timed_out = True
            return_code = getattr(error, "returncode", None)
            error_message = f"Experiment exceeded timeout of {resolved.timeout_seconds} seconds"
        except OSError as error:
            error_message = str(error)

        elapsed_seconds = time.perf_counter() - started_clock
        finished_at = datetime.now(timezone.utc).isoformat()

        if timed_out:
            status = RunStatus.TIMED_OUT
        elif error_message is not None:
            status = RunStatus.FAILED
        elif return_code != 0:
            status = RunStatus.FAILED
            error_message = f"ChampSim exited with code {return_code}"
        elif not self._has_structured_outputs(resolved):
            status = RunStatus.INVALID_OUTPUT
            error_message = "ChampSim exited successfully but structured metric outputs are missing or empty"
        else:
            status = RunStatus.SUCCEEDED

        result = RunResult(
            spec=resolved,
            status=status,
            return_code=return_code,
            timed_out=timed_out,
            elapsed_seconds=elapsed_seconds,
            command=command,
            started_at=started_at,
            finished_at=finished_at,
            error_message=error_message,
        )
        self._write_json(resolved.result_path, result.to_dict())
        return result

    def _validate_inputs(self, spec: ExperimentSpec) -> None:
        if not spec.binary.is_file():
            raise FileNotFoundError(f"ChampSim binary not found: {spec.binary}")
        if not spec.trace.is_file():
            raise FileNotFoundError(f"Trace not found: {spec.trace}")
        if spec.config_path is not None and not spec.config_path.is_file():
            raise FileNotFoundError(f"Configuration not found: {spec.config_path}")

    @staticmethod
    def _has_structured_outputs(spec: ExperimentSpec) -> bool:
        return (
            spec.metrics_jsonl_path.is_file()
            and spec.metrics_jsonl_path.stat().st_size > 0
            and spec.metric_definitions_path.is_file()
            and spec.metric_definitions_path.stat().st_size > 0
        )

    @staticmethod
    def _write_json(path: Path, value: dict) -> None:
        with path.open("w", encoding="utf-8") as output:
            json.dump(value, output, indent=2, ensure_ascii=False)
            output.write("\n")



