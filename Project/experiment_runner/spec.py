"""Immutable description of one ChampSim experiment run."""

from __future__ import annotations

import re
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Callable, Optional, Tuple


_RUN_ID_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")


@dataclass(frozen=True)
class ExperimentSpec:
    """All inputs needed to launch one isolated ChampSim process.

    The object deliberately contains no process state.  It describes *what*
    should run, while ExperimentRunner decides *how* to execute it.
    """

    run_id: str
    binary: Path
    trace: Path
    output_dir: Path
    timeout_seconds: Optional[float] = 3600.0
    warmup_instructions: Optional[int] = None
    simulation_instructions: Optional[int] = None
    extra_args: Tuple[str, ...] = ()
    launcher_args: Tuple[str, ...] = ()
    config_path: Optional[Path] = None

    def __post_init__(self) -> None:
        if not _RUN_ID_PATTERN.fullmatch(self.run_id):
            raise ValueError(
                "run_id must start with a letter or digit and contain only "
                "letters, digits, '.', '_' or '-'"
            )

        object.__setattr__(self, "binary", Path(self.binary))
        object.__setattr__(self, "trace", Path(self.trace))
        object.__setattr__(self, "output_dir", Path(self.output_dir))
        object.__setattr__(self, "extra_args", tuple(str(arg) for arg in self.extra_args))
        object.__setattr__(self, "launcher_args", tuple(str(arg) for arg in self.launcher_args))

        if self.config_path is not None:
            object.__setattr__(self, "config_path", Path(self.config_path))

        if self.timeout_seconds is not None and self.timeout_seconds <= 0:
            raise ValueError("timeout_seconds must be positive or None")
        if self.warmup_instructions is not None and self.warmup_instructions < 0:
            raise ValueError("warmup_instructions must be non-negative")
        if self.simulation_instructions is not None and self.simulation_instructions <= 0:
            raise ValueError("simulation_instructions must be positive")

    @property
    def metrics_jsonl_path(self) -> Path:
        return self.output_dir / "metrics.jsonl"

    @property
    def metric_definitions_path(self) -> Path:
        return self.output_dir / "metric_definitions.json"

    @property
    def stdout_path(self) -> Path:
        return self.output_dir / "stdout.log"

    @property
    def stderr_path(self) -> Path:
        return self.output_dir / "stderr.log"

    @property
    def run_spec_path(self) -> Path:
        return self.output_dir / "run_spec.json"

    @property
    def result_path(self) -> Path:
        return self.output_dir / "run_result.json"

    @property
    def copied_config_path(self) -> Path:
        return self.output_dir / "experiment.config.json"

    def resolve(self, base_dir: Path) -> "ExperimentSpec":
        """Return a copy whose filesystem paths are relative to base_dir."""

        base_dir = Path(base_dir)

        def absolute(path: Path) -> Path:
            return path if path.is_absolute() else base_dir / path

        return replace(
            self,
            binary=absolute(self.binary),
            trace=absolute(self.trace),
            output_dir=absolute(self.output_dir),
            config_path=None if self.config_path is None else absolute(self.config_path),
        )

    def build_command(self, path_mapper: Optional[Callable[[Path], str]] = None) -> Tuple[str, ...]:
        """Build the ChampSim command without invoking a shell.

        path_mapper lets an execution backend translate host paths into paths
        visible inside a container. Native execution uses the identity mapping.
        """

        map_path = path_mapper if path_mapper is not None else lambda path: str(path)
        command = [
            map_path(self.binary),
            *self.launcher_args,
            "--run-id",
            self.run_id,
            "--metrics-jsonl",
            map_path(self.metrics_jsonl_path),
            "--metric-definitions",
            map_path(self.metric_definitions_path),
        ]

        if self.warmup_instructions is not None:
            command.extend(["--warmup-instructions", str(self.warmup_instructions)])
        if self.simulation_instructions is not None:
            command.extend(["--simulation-instructions", str(self.simulation_instructions)])

        command.extend(self.extra_args)
        command.append(map_path(self.trace))
        return tuple(command)

    def to_dict(self) -> dict:
        return {
            "run_id": self.run_id,
            "binary": str(self.binary),
            "trace": str(self.trace),
            "output_dir": str(self.output_dir),
            "timeout_seconds": self.timeout_seconds,
            "warmup_instructions": self.warmup_instructions,
            "simulation_instructions": self.simulation_instructions,
            "extra_args": list(self.extra_args),
            "launcher_args": list(self.launcher_args),
            "config_path": None if self.config_path is None else str(self.config_path),
        }

