# Program Dependence Graph in LLVM

## Overview

A Program Dependence Graph (PDG) is a *program representation* that conveys both data and control dependencies. In a PDG, nodes are usually instructions and edges between these nodes indicate some form of dependence. Edges can represent a data dependence, control dependence, or call dependence. One way to build a PDG is to build a Control Dependence Graph (CDG) and a Dependence Graph (DDG) independently. The CDG captures the control dependencies within the program, while the DDG captures the data dependencies. Combining these two graphs yields the PDG. Recent research has leveraged PDGs to aid in *program slicing*, which is the process of "slicing" an existing (usually monolithic) program into separate partitions. Slices of a program can be identified by disjoint partitions in the PDG. If a partition *A* of a PDG is disjoint from another partition *B*, then the instructions contained within should not have any control or data dependencies. This means they can run orthogonally, or in their own PIRATE enclaves. Program Dependence Graphs have other applications, such as finding program dependencies for the sake of  parallelization or optimization of code. 

### Data Dependence

### Control Dependence

## Prior Work

## Design

### DDG
### CDG
### PDG

## References


