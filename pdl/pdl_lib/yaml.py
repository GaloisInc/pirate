"""The module responsible for working with the actual YAML files."""
from io import open
from os.path import isfile, join
from sys import exit  # pylint: disable=redefined-builtin
from typing import Dict

from cerberus import Validator
from yaml import safe_load, dump

from .project import ProjectConfiguration

_channel_resource_schema: Dict = {
    'type': 'dict',
    'schema': {
        'name': {'type': 'string'},
        'ids': {
            'type': 'list',
            'schema': {'type': 'string'}
        },
        'type': {'type': 'string', 'allowed': ['channel']},
        'contents': {
            'type': 'dict',
            'schema': {
                'channel_type': {'type': 'string'},
                'path': {'type': 'string'}
            }
        }
    }
}

_integer_resource_schema: Dict = {
    'type': 'dict',
    'schema': {
        'name': {'type': 'string'},
        'ids': {
            'type': 'list',
            'schema': {'type': 'string'}
        },
        'type': {'type': 'string', 'allowed': ['integer']},
        'contents': {'type': 'integer'}
    }
}

_boolean_resource_schema: Dict = {
    'type': 'dict',
    'schema': {
        'name': {'type': 'string'},
        'ids': {
            'type': 'list',
            'schema': {'type': 'string'}
        },
        'type': {'type': 'string', 'allowed': ['boolean']},
        'contents': {'type': 'boolean'} 
    }
}

_enclave_schema: Dict = {
    'type': 'dict',
    'schema': {
        'name': {
            'type': 'string'},
        'path': {
            'type': 'string'},
        'host': {
            'type': 'string'},
        'resources': {
            'type': 'list',
            'schema': {
                'anyof': [_integer_resource_schema, _boolean_resource_schema, _channel_resource_schema]
            }
        }
    }
}

_host_schema: Dict = {
    'type': 'dict',
    'schema': {
        'name': {
            'required': True,
            'type': 'string'},
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
        'schema': _enclave_schema},
    'hosts': {
        'type': 'list',
        'schema': _host_schema},
    'agents': {
        'type': 'list',
        'schema': _agent_schema},
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
    },
    'startup_order': {
        'type': 'list',
        'required': False,
        'schema': {
            'type': 'string'
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


def generate_enc_pal_file(path: str, enc: str,
                          prj: ProjectConfiguration) -> None:
    """Generate PAL YAML files for each enclave.

    Args:
        path (str): Directory where file should be placed
        enc (str): The enclave for which we're generating a launcher file
        prj (ProjectConfiguration): Project configuration to  generate
    """
    stream = open(join(path, 'pal.yaml'), 'w')
    dump({'enclaves': {'name': 'test_enclave', 'path': 'test_path'}}, stream)


def generate_debug_pal_file(path: str, prj: ProjectConfiguration) -> None:
    """Generate a single PAL YAML file for debugging, which forces everything
    to be local.

    Args:
        path (str): The directory where the file should be placed
        prj (ProjectConfiguration): The project for which to generate the configuration
    """
    with open(join(path, 'pdl.yaml'), 'w') as f:
        f.write("#!/usr/bin/env pal\n\n")
        yaml: Dict = {}
        yaml['enclaves'] = []
        yaml['resources'] = []
        for enc in prj.enclaves:
            enc_dict: Dict = {'name': enc.name}
            yaml["enclaves"].append(enc_dict)

            for res in enc.resources:
                res_dict: Dict = {
                    'name': res.name,
                    'ids': [],
                    'type': res.type
                }
                for ident in res.ids:
                    res_dict['ids'].append(ident)
                if res.type == 'channel':
                    res_dict['contents'] = {
                        'channel_type': 'pipe',
                        'path': res.path
                    }
                elif res.type == 'integer':
                    res_dict['contents'] = res.contents_int
                elif res.type == 'boolean':
                    res_dict['contents'] = res.contents_bool
                yaml['resources'].append(res_dict)

        yaml['config'] = {'log_level': prj.config.log_level}

        dump(yaml, f)
