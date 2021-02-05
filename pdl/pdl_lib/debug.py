"""Support for debugging pirate programs locally.

Doesn't actually do the executing, but packages things up and dumps out
the requisite info in a machine-readable format.
"""

from argparse import Namespace
from os import symlink
from os.path import isfile, join
from shutil import copy
from distutils.dir_util import copy_tree
from sys import exit  # pylint: disable=redefined-builtin
from tempfile import TemporaryDirectory
from typing import List

from .project import ProjectConfiguration
from .yaml import generate_debug_pal_files


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
        if not enc_name in prj.enclave_names():
            exit("Startup order includes unknown enclave: " + enc_name)

    # Then append any items that aren't specified
    for known_enc in prj.enclave_names():
        if not known_enc in complete_startup_order:
            print(
                "Startup order for enclave " +
                known_enc +
                " not specified.  Adding to end.")
            complete_startup_order.append(known_enc)

    return complete_startup_order


def debug(prj: ProjectConfiguration, args: Namespace) -> None:
    """Execute a project configuration locally.

    Args:
        prj (ProjectConfiguration): The project to execute
    """
    order = total_startup_order(prj)
    with TemporaryDirectory() as tmpdir:
        # Copy all executables to the temp dir
        for enc in order:
            path = prj.enclaves_by_name[enc].path
            if not isfile(path):
                exit("Enclave binary at " + path + " doesn't exist")
            symlink(path, join(tmpdir, enc))
            # copy(path, join(tmpdir, enc))

        generate_debug_pal_files(tmpdir, prj)

        with open(join(tmpdir, 'startup_order'), 'w') as file:
            for enc in prj.startup_order:
                file.write(enc)
                file.write('\n')

        copy_tree(tmpdir, args.output_dir)
