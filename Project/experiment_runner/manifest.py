"""Loading declarative JSON manifests for experiment batches."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from .batch import ExperimentBatchSpec
from .spec import ExperimentSpec


_TOP_LEVEL_FIELDS = {
    "batch_id",
    "output_dir",
    "max_workers",
    "baseline_run_id",
    "defaults",
    "runs",
}
_DEFAULT_FIELDS = {
    "trace",
    "binary",
    "timeout_seconds",
    "warmup_instructions",
    "simulation_instructions",
    "extra_args",
    "launcher_args",
    "config_path",
}
_RUN_FIELDS = _DEFAULT_FIELDS | {
    "run_id",
    "output_dir",
}
_PATH_FIELDS = {"trace", "binary", "output_dir", "config_path"}


@dataclass(frozen=True)
class LoadedBatchManifest:
    """A parsed batch plus report settings declared in the same manifest."""

    batch: ExperimentBatchSpec
    baseline_run_id: Optional[str]
    manifest_path: Path


class BatchManifestLoader:
    """Parse a JSON manifest into ExperimentBatchSpec and ExperimentSpec objects."""

    def load(self, manifest_path: Path) -> LoadedBatchManifest:
        manifest_path = Path(manifest_path)
        with manifest_path.open("r", encoding="utf-8") as input_file:
            payload = json.load(input_file)
        if not isinstance(payload, dict):
            raise ValueError("Batch manifest root must be a JSON object")

        unknown_top_level = set(payload) - _TOP_LEVEL_FIELDS
        if unknown_top_level:
            raise ValueError(
                "Unknown top-level manifest fields: " + ", ".join(sorted(unknown_top_level))
            )

        for required in ("batch_id", "output_dir", "runs"):
            if required not in payload:
                raise ValueError(f"Batch manifest is missing required field: {required}")

        defaults = payload.get("defaults", {})
        if not isinstance(defaults, dict):
            raise ValueError("Manifest defaults must be a JSON object")
        unknown_defaults = set(defaults) - _DEFAULT_FIELDS
        if unknown_defaults:
            raise ValueError(
                "Unknown default fields: " + ", ".join(sorted(unknown_defaults))
            )

        raw_runs = payload["runs"]
        if not isinstance(raw_runs, list) or not raw_runs:
            raise ValueError("Manifest runs must be a non-empty JSON array")

        runs = []
        run_ids = []
        for index, raw_run in enumerate(raw_runs):
            if not isinstance(raw_run, dict):
                raise ValueError(f"Run #{index + 1} must be a JSON object")
            unknown_run_fields = set(raw_run) - _RUN_FIELDS
            if unknown_run_fields:
                raise ValueError(
                    f"Unknown fields in run #{index + 1}: "
                    + ", ".join(sorted(unknown_run_fields))
                )

            merged = dict(defaults)
            merged.update(raw_run)
            for required in ("run_id", "binary", "trace", "output_dir"):
                if required not in merged:
                    raise ValueError(
                        f"Run #{index + 1} is missing required field after defaults: {required}"
                    )

            for field in _PATH_FIELDS:
                if field in merged and merged[field] is not None:
                    merged[field] = Path(merged[field])

            try:
                run = ExperimentSpec(**merged)
            except (TypeError, ValueError) as error:
                raise ValueError(f"Invalid run #{index + 1}: {error}") from error
            runs.append(run)
            run_ids.append(run.run_id)

        baseline_run_id = payload.get("baseline_run_id")
        if baseline_run_id is not None:
            baseline_run_id = str(baseline_run_id)
            if baseline_run_id not in run_ids:
                raise ValueError(
                    f"baseline_run_id {baseline_run_id!r} is not present in manifest runs"
                )

        batch = ExperimentBatchSpec(
            batch_id=str(payload["batch_id"]),
            output_dir=Path(payload["output_dir"]),
            runs=tuple(runs),
            max_workers=int(payload.get("max_workers", 1)),
        )
        return LoadedBatchManifest(
            batch=batch,
            baseline_run_id=baseline_run_id,
            manifest_path=manifest_path,
        )
