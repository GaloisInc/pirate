"""This module sets up CLI argument parsing."""

import argparse


def parse_arguments() -> argparse.Namespace:
    """The entry point for performing argument parsing."""
    parser = argparse.ArgumentParser(
        description='A tool for testing and deploying Pirate programs')
    parser.add_argument(
        "dir",
        help="The path to the project")
    parser.add_argument(
        "command",
        help="What to do with the project",
        choices=[
            'tar',
            'execute',
            'simulate'])
    return parser.parse_args()
