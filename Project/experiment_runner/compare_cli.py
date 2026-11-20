"""Command-line entry point for comparing a completed experiment batch."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Optional

from .charts import ComparisonChartRenderer
from .comparison import (
    ComparisonReportWriter,
    ExperimentComparator,
    ResultCollector,
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Compare structured ChampSim metrics")
    parser.add_argument(
        "--batch-dir",
        required=True,
        type=Path,
        help="Directory containing batch_result.json, or the JSON file itself",
    )
    parser.add_argument(
        "--baseline-run-id",
        required=True,
        help="run_id used as the comparison baseline",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="Directory for comparison.json and comparison.md; defaults to the batch directory",
    )
    parser.add_argument(
        "--chart-dir",
        type=Path,
        help="Directory for PNG charts; defaults to <output-dir>/charts",
    )
    parser.add_argument(
        "--no-charts",
        action="store_true",
        help="Skip PNG chart generation",
    )
    return parser


def main(argv: Optional[list[str]] = None) -> int:
    args = build_parser().parse_args(argv)
    collector = ResultCollector()
    batch = collector.collect_batch(args.batch_dir)
    report = ExperimentComparator().compare(batch, args.baseline_run_id)

    if args.output_dir is not None:
        output_dir = args.output_dir
    else:
        output_dir = batch.batch_result_path.parent
    json_path, markdown_path = ComparisonReportWriter(output_dir).write(report)

    chart_paths = {}
    if not args.no_charts:
        chart_dir = args.chart_dir if args.chart_dir is not None else output_dir / "charts"
        chart_paths = {
            name: str(path)
            for name, path in ComparisonChartRenderer(chart_dir).render(report).items()
        }

    print(
        json.dumps(
            {
                "batch_id": batch.batch_id,
                "baseline_run_id": report.baseline_run_id,
                "candidate_run_ids": [item.candidate_run_id for item in report.candidates],
                "comparison_json": str(json_path),
                "comparison_markdown": str(markdown_path),
                "charts": chart_paths,
            },
            indent=2,
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())


