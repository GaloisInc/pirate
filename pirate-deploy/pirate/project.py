"""Defines types and helpers that represent a project configuration.

Essentially the intermediate representation between the input YAML and
the PAL-friendly output.
"""

from typing import Any, Dict, List


class AgentEndpoint:
    """Identifies one of two endpoints of an agent/route combination."""

    def __init__(self, agent: str, name: str):
        self.agent = agent
        self.route_name = name


def make_agent_endpoint(raw_text: str) -> AgentEndpoint:
    """A smart constructor for agent endpoints that handles strings in the
    format we get from raw YAML.

    Args:
        rawText (str): [description]

    Returns:
        AgentEndpoint: [description]
    """
    [agent, route] = raw_text.split('.')
    return AgentEndpoint(agent, route)


class Route:
    """Essentially a pair of endpoints."""

    def __init__(self, src: AgentEndpoint, dst: AgentEndpoint):
        self.src = src
        self.dst = dst

    def __str__(self) -> str:
        return "route from " + self.src.agent + "/" + self.src.route_name + \
            " to " + self.dst.agent + "/" + self.dst.route_name


class Timer:
    """Timers that need to be generated for the application."""

    def __init__(self, frequency: str, target_agent: str, target_id: str):
        """Construct a Timer.

        Args:
            frequency (str): The frequency of ticks
            target_agent (str): The name of the agent capturing tick events
            target_id (str): The identifier in the code of the agent
        """
        self.frequency = frequency
        self.target_agent = target_agent
        self.target_id = target_id


class ProjectConfiguration:
    """The main type that stores all the config data for a project."""

    def __init__(self, yaml: Any) -> None:
        self.yaml = yaml
        self.enclave_to_path: Dict[str, str] = {}
        self.project: Dict[str, Dict[str, List[Route]]] = {}

        # Add all the enclave names as keys to the outermost dictionary.  This is
        # only necessary if we have enclaves that don't have any agents
        for enc_dict in yaml['enclaves']:
            enc_name = enc_dict['name']
            self.enclave_to_path[enc_name] = enc_dict['path']
            self.project[enc_name] = {}

        # Now add agents in to matching enclave.  Also track the agent to
        # enclave mapping for lookup
        self.agents_to_enclave: Dict[str, str] = {}
        for agent in yaml['agents']:
            agent_name = agent['name']
            agent_enclave = agent['enclave']
            curr = self.project[agent_enclave]
            curr[agent_name] = []
            self.project[agent_enclave] = curr
            self.agents_to_enclave[agent_name] = agent_enclave

        for route in yaml['routes']:
            src = make_agent_endpoint(route['source'])
            dst = make_agent_endpoint(route['target'])
            src_enc = self.agents_to_enclave[src.agent]
            curr_outgoing = self.project[src_enc][src.agent]
            curr_outgoing.append(Route(src, dst))
            self.project[src_enc][src.agent] = curr_outgoing

        self.timers: List[Timer] = []
        for timer in yaml['timers']:
            [target_agent, target_id] = timer['target'].split(".")
            self.timers.append(
                Timer(
                    timer['frequency'],
                    target_agent,
                    target_id))

    def dump(self) -> None:
        """For debug purposes, print out a text representation of a project
        configuration."""
        print("Project Configuration")
        print("---------------------")
        for enc in self.project:
            print("Enclave: ", enc)
            for agent in self.project[enc]:
                print("  - ", agent)
                for route in self.project[enc][agent]:
                    print("    + ", str(route))
