"""One-command batch execution, comparison, and chart generation."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Optional

from .batch import ExperimentBatchRunner, BatchStatus
from .charts import ComparisonChartRenderer
from .comparison import ComparisonReportWriter, ExperimentComparator, ResultCollector
from .execution import DockerExecutionBackend, NativeExecutionBackend
from .manifest import BatchManifestLoader


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run a declarative ChampSim experiment batch and generate reports"
    )
    parser.add_argument("--manifest", required=True, type=Path, help="Batch manifest JSON file")
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path.cwd(),
        help="Base directory for relative paths in the manifest",
    )
    parser.add_argument(
        "--baseline-run-id",
        help="Override the manifest baseline run_id",
    )
    parser.add_argument("--docker-image", help="Run Linux ChampSim binaries in this Docker image")
    parser.add_argument("--docker-workspace", default="/workspace")
    parser.add_argument("--docker-executable", default="docker")
    parser.add_argument("--no-compare", action="store_true", help="Skip comparison and charts")
    parser.add_argument("--no-charts", action="store_true", help="Generate comparison files but not PNG charts")
    parser.add_argument("--chart-dir", type=Path, help="Override PNG chart output directory")
    return parser


def main(argv: Optional[list[str]] = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        loaded = BatchManifestLoader().load(args.manifest)
        backend = NativeExecutionBackend()
        if args.docker_image is not None:
            backend = DockerExecutionBackend(
                image=args.docker_image,
                workspace=args.docker_workspace,
                docker_executable=args.docker_executable,
            )

        batch_result = ExperimentBatchRunner(
            args.project_root,
            execution_backend=backend,
        ).run(loaded.batch)

        summary = {
            "batch_id": batch_result.spec.batch_id,
            "status": batch_result.status.value,
            "elapsed_seconds": batch_result.elapsed_seconds,
            "succeeded_count": batch_result.succeeded_count,
            "failed_count": batch_result.failed_count,
            "batch_result": str(batch_result.spec.batch_result_path),
            "comparison_json": None,
            "comparison_markdown": None,
            "charts": {},
        }

        if not args.no_compare and len(batch_result.spec.runs) > 1:
            baseline_run_id = (
                args.baseline_run_id
                or loaded.baseline_run_id
                or batch_result.spec.runs[0].run_id
            )
            successful_run_ids = {result.spec.run_id for result in batch_result.results if result.succeeded}
            if baseline_run_id not in successful_run_ids:
                raise ValueError(
                    f"Baseline run {baseline_run_id!r} did not complete successfully"
                )

            collected_batch = ResultCollector().collect_batch(batch_result.spec.batch_result_path)
            report = ExperimentComparator().compare(collected_batch, baseline_run_id)
            json_path, markdown_path = ComparisonReportWriter(
                batch_result.spec.output_dir
            ).write(report)
            summary["comparison_json"] = str(json_path)
            summary["comparison_markdown"] = str(markdown_path)

            if not args.no_charts:
                chart_dir = args.chart_dir if args.chart_dir is not None else batch_result.spec.output_dir / "charts"
                summary["charts"] = {
                    name: str(path)
                    for name, path in ComparisonChartRenderer(chart_dir).render(report).items()
                }

        print(json.dumps(summary, indent=2, ensure_ascii=False))
        return 0 if batch_result.status == BatchStatus.SUCCEEDED else 1
    except Exception as error:
        print(
            json.dumps(
                {
                    "status": "error",
                    "error_type": type(error).__name__,
                    "message": str(error),
                },
                indent=2,
                ensure_ascii=False,
            )
        )
        return 1


if __name__ == "__main__":
    sys.exit(main())
