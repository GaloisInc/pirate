"""This is the entry point of the library that makes up pirate-deploy.

That is, the actual script to run is in the bin directory, while this
one is what it calls into.
"""
import tarfile
from os.path import isfile, split, join
from shutil import copy
from sys import exit  # pylint: disable=redefined-builtin
from tempfile import TemporaryDirectory

from .arguments import parse_arguments
from .execute import execute
from .project import ProjectConfiguration
from .simulate import simulate
from .yaml import load_yaml, generate_pal_file


def collect_and_tar(enc: str, path: str, prj: ProjectConfiguration) -> str:
    """For a given enclave and file, package it up with the corresponding PAL file

    Args:
        enc (str): The name of the current enclave
        path (str): The path to the enclave binary
        prj (ProjectConfiguration): The project configuration, to generate PAL files

    Returns:
        str: [description]
    """

    if not isfile(path):
        exit("Enclave binary at " + path + " doesn't exist")
    with TemporaryDirectory() as tmpdir:
        # TODO figure out how to deal with relative paths, running in different
        # dirs, etc.
        (_, fname) = split(path)
        copy(path, join(tmpdir, fname))
        generate_pal_file(tmpdir, enc, prj)
        with tarfile.open(enc + ".tgz", "w:gz") as tar:
            tar.add(tmpdir, arcname=enc)
            return enc + ".tgz"


def tar_all(prj: ProjectConfiguration) -> None:
    """Find all of the enclaves and package them up for distribution

    Args:
        prj (ProjectConfiguration): The project we are packaging up
    """
    print("zipping")
    for enc in prj.enclave_to_path:
        collect_and_tar(enc, prj.enclave_to_path[enc], prj)


def main() -> None:
    """Serves as an entry point for the CLI script."""
    args = parse_arguments()
    prj = load_yaml(args.dir)
    if args.command == 'tar':
        tar_all(prj)
    elif args.command == 'simulate':
        simulate(prj)
    elif args.command == 'execute':
        execute(prj)
