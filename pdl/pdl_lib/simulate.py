"""Module for simulating a pirate project deployment using Docker-Compose."""

from .project import ProjectConfiguration


def simulate(prj: ProjectConfiguration) -> None:
    """Simulate a deployment using Docker-Compose.

    Args:
        prj (ProjectConfiguration): [description]
    """
    print("Simulating")
    prj.dump()
