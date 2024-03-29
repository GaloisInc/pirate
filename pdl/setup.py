import setuptools

setuptools.setup(
    name="pdl-pkg",
    version="0.0.1",
    author="Pirate Team",
    python_requires='>=3.6',
    packages=setuptools.find_packages(),
    scripts=["bin/pdl"],
    install_requires=[
        "pyyaml==5.3.1",
        "networkx==2.5",
        "cerberus==1.3.2",
    ]
)
