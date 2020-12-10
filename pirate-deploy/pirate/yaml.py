"""The module responsible for working with the actual YAML files."""
from io import open
from os.path import isfile, join
from sys import exit  # pylint: disable=redefined-builtin
from typing import Dict

from cerberus import Validator
from yaml import safe_load, dump

from .project import ProjectConfiguration

_resource_schema: Dict = {
    'type': 'dict',
    # Resources can be a string, bool, or integer
    'anyof_schema': [{
        'id': {
            'type': 'string',
            'required': True},
        'value': {
            'type': 'string',
            'required': True}
    }, {
        'id': {
            'type': 'string',
            'required': True},
        'value': {
            'type': 'boolean',
            'required': True}
    }, {
        'id': {
            'type': 'string',
            'required': True},
        'value': {
            'type': 'integer',
            'required': True}
    }]
}

_enclave_schema: Dict = {
    'type': 'dict',
    'schema': {
        'name': {
            'required': True,
            'type': 'string'},
        'path': {
            'required': True,
            'type': 'string'},
        'resources': {
            'type': 'list',
            'schema': _resource_schema},
        'location': {
            'required': True,
            'type': 'string'}
    }
}

_agent_schema: Dict = {
    'type': 'dict',
    'schema': {
        'name': {
            'type': 'string',
            'required': True},
        'enclave': {
            'type': 'string',
            'required': True}
    }
}

_timer_schema: Dict = {
    'type': 'dict',
    'schema': {
        'frequency': {
            'type': 'integer',
            'required': True},
        'target': {
            'type': 'string',
            'required': True}
    }
}

_route_schema: Dict = {
    'type': 'dict',
    'schema': {
        'source': {
            'type': 'string',
            'required': True},
        'target': {
            'type': 'string',
            'required': True}
    }
}

_schema: Dict = {
    'enclaves': {
        'type': 'list',
        'schema': _enclave_schema,
        'required': True},
    'agents': {
        'type': 'list',
        'schema': _agent_schema,
        'required': True},
    'timers': {
        'type': 'list',
        'schema': _timer_schema},
    'routes': {
        'type': 'list',
        'schema': _route_schema},
    'config': {
        'type': 'dict',
        'schema': {
            'log_level': {
                'type': 'string',
            }
        }
    }
}


def load_yaml(yaml_file: str) -> ProjectConfiguration:
    """Tries to open the given filepath and parse it as a YAML file.

    Args:
        yaml_file (str): the filepath to the YAML file

    Returns:
        ProjectConfiguration: the object representing the parsed configuration
    """
    if not isfile(yaml_file):
        exit("The specified YAML file was not found")
    data = safe_load(open(yaml_file, "r", encoding="utf-8"))
    val = Validator(_schema)
    if not val.validate(data):
        print(val.errors)
        exit("Exited due to YAML schema validation fail")
    return ProjectConfiguration(data)


def generate_pal_file(path: str, enc: str, prj: ProjectConfiguration) -> None:
    """[summary]

    Args:
        path (str): Directory where file should be placed
        enc (str): The enclave for which we're generating a launcher file
        prj (ProjectConfiguration): Project configuration to  generate
    """
    stream = open(join(path, 'pal.yaml'), 'w')
    dump({'enclaves': {'name': 'test_enclave', 'path': 'test_path'}}, stream)
