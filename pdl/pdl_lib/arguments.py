"""This module sets up CLI argument parsing."""

import argparse


def parse_arguments() -> argparse.Namespace:
    """The entry point for performing argument parsing."""
    parser = argparse.ArgumentParser(
        prog='pdl',
        description='A tool for testing and deploying Pirate programs')
    parser.add_argument(
        "yaml_config",
        help="The path to the YAML configuration file")
    parser.add_argument(
        "command",
        help="What to do with the project",
        choices=[
            'deploy',
            'debug'])
    return parser.parse_args()
