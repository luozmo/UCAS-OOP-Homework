"""Execution backends for native and Docker-based ChampSim runs."""

from __future__ import annotations

from abc import ABC, abstractmethod
from pathlib import Path
from typing import Tuple

from .spec import ExperimentSpec


class ExecutionBackend(ABC):
    """Translate one ExperimentSpec into an executable command."""

    @abstractmethod
    def command(self, spec: ExperimentSpec, host_root: Path) -> Tuple[str, ...]:
        raise NotImplementedError


class NativeExecutionBackend(ExecutionBackend):
    """Run a simulator binary directly on the host."""

    def command(self, spec: ExperimentSpec, host_root: Path) -> Tuple[str, ...]:
        return spec.build_command()


class DockerExecutionBackend(ExecutionBackend):
    """Run a Linux simulator inside Docker while writing artifacts on the host.

    The whole project directory is mounted into the container. Host paths are
    translated to their equivalent paths under the container workspace.
    """

    def __init__(
        self,
        image: str,
        workspace: str = "/workspace",
        docker_executable: str = "docker",
    ) -> None:
        if not image:
            raise ValueError("Docker image must not be empty")
        if not workspace.startswith("/"):
            raise ValueError("Docker workspace must be an absolute container path")
        self.image = image
        self.workspace = workspace.rstrip("/") or "/"
        self.docker_executable = docker_executable

    def command(self, spec: ExperimentSpec, host_root: Path) -> Tuple[str, ...]:
        host_root = Path(host_root).resolve()

        def container_path(host_path: Path) -> str:
            resolved_path = Path(host_path).resolve()
            try:
                relative_path = resolved_path.relative_to(host_root)
            except ValueError as error:
                raise ValueError(
                    f"Path {resolved_path} is outside the Docker-mounted project root {host_root}"
                ) from error
            return str(Path(self.workspace) / relative_path).replace("\\", "/")

        simulator_command = spec.build_command(path_mapper=container_path)
        return (
            self.docker_executable,
            "run",
            "--rm",
            "-v",
            f"{host_root}:{self.workspace}",
            "-w",
            self.workspace,
            self.image,
            *simulator_command,
        )
