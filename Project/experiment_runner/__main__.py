"""Command-line entry point for one isolated ChampSim run."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .execution import DockerExecutionBackend, NativeExecutionBackend
from .runner import ExperimentRunner
from .spec import ExperimentSpec


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run one isolated ChampSim experiment")
    parser.add_argument("--binary", required=True, type=Path, help="ChampSim executable")
    parser.add_argument("--trace", required=True, type=Path, help="Trace passed to ChampSim")
    parser.add_argument("--config", type=Path, help="Optional JSON config copied into the run directory")
    parser.add_argument("--run-id", required=True, help="Unique identifier written into structured metrics")
    parser.add_argument("--output-dir", required=True, type=Path, help="New directory for this run's artifacts")
    parser.add_argument("--timeout-seconds", type=float, default=3600.0, help="Wall-clock timeout; 0 disables it")
    parser.add_argument("--warmup-instructions", type=int)
    parser.add_argument("--simulation-instructions", type=int)
    parser.add_argument("--hide-heartbeat", action="store_true", help="Pass --hide-heartbeat to ChampSim")
    parser.add_argument("--docker-image", help="Run a Linux ChampSim binary inside this Docker image")
    parser.add_argument("--docker-workspace", default="/workspace", help="Container mount point for the project")
    parser.add_argument("--docker-executable", default="docker", help="Docker client executable")
    parser.add_argument(
        "--extra-arg",
        action="append",
        default=[],
        help="Additional simulator argument; repeat for multiple arguments",
    )
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path.cwd(),
        help="Base directory for relative paths (default: current directory)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    timeout = None if args.timeout_seconds == 0 else args.timeout_seconds
    extra_args = list(args.extra_arg)
    if args.hide_heartbeat:
        extra_args.append("--hide-heartbeat")

    spec = ExperimentSpec(
        run_id=args.run_id,
        binary=args.binary,
        trace=args.trace,
        output_dir=args.output_dir,
        timeout_seconds=timeout,
        warmup_instructions=args.warmup_instructions,
        simulation_instructions=args.simulation_instructions,
        extra_args=tuple(extra_args),
        config_path=args.config,
    )
    backend = NativeExecutionBackend()
    if args.docker_image is not None:
        backend = DockerExecutionBackend(
            image=args.docker_image,
            workspace=args.docker_workspace,
            docker_executable=args.docker_executable,
        )

    result = ExperimentRunner(args.project_root, execution_backend=backend).run(spec)
    print(json.dumps(result.to_dict(), indent=2, ensure_ascii=False))
    return 0 if result.succeeded else 1


if __name__ == "__main__":
    sys.exit(main())


