"""This module sets up CLI argument parsing."""

import argparse


def parse_arguments_old() -> argparse.Namespace:
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


def parse_arguments() -> argparse.Namespace:
    """Argument parsing with subcommands"""
    parser = argparse.ArgumentParser(
        prog='pdl',
        description='A tool for testing and deploying Pirate programs')
    subparsers = parser.add_subparsers(help='commands', dest="command")
    parser.add_argument(
        "yaml_config",
        help="Path to the YAML config, if not pdl.yaml")
    debug_parser = subparsers.add_parser("debug")
    deploy_parser = subparsers.add_parser("deploy")
    debug_parser.add_argument(
        "output_dir",
        help="where to put debug files")

    return parser.parse_args()
