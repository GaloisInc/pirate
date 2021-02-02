"""Defines types and helpers that represent a project configuration.

Essentially the intermediate representation between the input YAML and
the PAL-friendly output.
"""

from typing import Any, Dict, List, KeysView

class Resource:
    """A PAL-managed resource"""
    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml
        self.name = yaml['name']
        self.ids : List[str] = yaml['ids']
        self.type = yaml['type']
        if self.type == 'channel':
            self.channel_type = yaml['contents']['channel_type']
            self.path = yaml['contents']['path']
        elif self.type == 'integer':
            self.contents_int: int = yaml['contents']
        elif self.type == 'boolean':
            self.contents_bool: bool = yaml['contents']


class Enclave:
    """Pirate Enclave"""
    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml
        self.name: str = yaml['name']
        self.path: str = yaml['path']
        self.resources: List[Resource] = []
        for res in yaml['resources']:
            self.resources.append(Resource(res))
        self.host = yaml['host']

class Host:
    """A machine on which one or more enclaves should run"""
    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml
        self.name: str = yaml['name']
        self.location: str = yaml['location']

class Agent:
    """An endpoint for a route, part of a single enclave"""
    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml
        self.name: str = yaml['name']
        self.enclave: str = yaml['enclave']

class Timer:
    """A timer we need to generate"""
    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml
        self.frequency: int = yaml['frequency']
        self.target: str = yaml['target']
        target_split = self.target.split('.')
        self.target_agent = target_split[0]
        self.target_function = target_split[1]

class Route:
    """A connection between two agents"""
    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml
        self.source = yaml['source']
        source_split = self.source.split('.')
        self.source_agent = source_split[0]
        self.source_function = source_split[1]

        self.target = yaml['target']
        target_split = self.target.split('.')
        self.target_agent = target_split[0]
        self.target_function = target_split[1]

class Config:
    """Additional miscellaneous configuration options for the project"""
    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml
        self.log_level = yaml['log_level']

class ProjectConfiguration:
    """The main type that stores all the config data for a project."""

    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml

        self.enclaves: List[Enclave] = []
        self.enclaves_by_name: Dict = {}
        for enc_yaml in yaml['enclaves']:
            enc = Enclave(enc_yaml)
            self.enclaves.append(enc)
            self.enclaves_by_name[enc.name] = enc

        self.hosts: List[Host] = []
        for host in yaml['hosts']:
            self.hosts.append(Host(host))

        self.agents: List[Agent] = []
        for agent in yaml['agents']:
            self.agents.append(Agent(agent))

        self.timers: List[Timer] = []
        for timer in yaml['timers']:
            self.timers.append(Timer(timer))

        self.routes: List[Route] = []
        for route in yaml['routes']:
            self.routes.append(Route(route))

        self.config: Config = Config(yaml['config'])

        self.startup_order: List[str] = yaml['startup_order']
    
    def enclave_names(self) -> KeysView[str]:
        """Return just the list of enclave names"""
        return self.enclaves_by_name.keys()
        

    def dump(self) -> None:
        """For debug purposes, print out a text representation of a project
        configuration."""
        print("Project Configuration")
        print("---------------------")
        for enc in self.enclaves:
            print("Enclave " + enc.name)
            for res in enc.resources:
                print("Resource " + res.name)
        print("")
        for route in self.routes:
            print("Route" + route.source_agent + " to " + route.target_agent)

