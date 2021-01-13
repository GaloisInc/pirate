"""Module for executing pirate programs locally."""

from asyncio import *
from sys import exit  # pylint: disable=redefined-builtin
from typing import List

from .project import ProjectConfiguration


def total_startup_order(prj: ProjectConfiguration) -> List[str]:
    """Because the YAML spec doesn't require the startup order to exist, nor to
    be complete if it does exist, we need to generate a order for all enclaves.

    Args:
        prj (ProjectConfiguration): [description]

    Returns:
        List[str]: [description]
    """
    complete_startup_order: List[str] = list(prj.startup_order)

    # First identify startup items that don't exist, which is a fatal error
    for enc_name in complete_startup_order:
        if not enc_name in prj.enclave_to_path.keys():
            exit("Startup order includes unknown enclave: " + enc_name)

    # Then append any items that aren't specified
    for known_enc in prj.enclave_to_path.keys():
        if not known_enc in complete_startup_order:
            print(
                "Startup order for enclave " +
                known_enc +
                " not specified.  Adding to end.")
            complete_startup_order.append(known_enc)

    return complete_startup_order


def execute(prj: ProjectConfiguration) -> None:
    """Execute a project configuration locally.

    Args:
        prj (ProjectConfiguration): The project to execute
    """
    order = total_startup_order(prj)
    for item in order:
        exe_path = prj.enclave_to_path[item]
        create_subprocess_exec(exe_path)
