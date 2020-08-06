====================================
Program Dependence Graphs in PIRATE
====================================

Introduction
-------------

A Program Dependence Graph (PDG) is a *program representation* that conveys
both data and control dependencies. In a PDG, nodes are usually instructions
and edges between these nodes indicate some form of dependence. Edges can
represent a data dependence, control dependence, or call dependence. One way
to build a PDG is to build a Control Dependence Graph (CDG) and a Dependence
Graph (DDG) independently. The CDG captures the control dependencies within
the program, while the DDG captures the data dependencies. Combining these
two graphs yields the PDG. Recent research has leveraged PDGs to aid in
*program slicing*, which is the process of "slicing" an existing (usually
monolithic) program into separate partitions. Slices of a program can be
identified by disjoint partitions in the PDG. If a partition *A* of a PDG is
disjoint from another partition *B*, then the instructions contained within
should not have any control or data dependencies. This means they can run
orthogonally, or alternatively, in their own PIRATE enclaves. Program
Dependence Graphs have other applications, such as finding program
dependencies for the sake of parallelization or optimization of code.

Control Dependence
+++++++++++++++++++

The following code exceprt shows a control dependence between statement `S1`
and statement `S2`. Statement `S2` is *control dependent* on statement `S1`
because `S2` will only execute if `S1` is false.

.. code-block:: language

    S1      if x > 2 goto L1 
    S2      y := 3 
    S3  L1: z := y + 1

Alternately phrased, `S1` is *not* post-dominated by `S2`, meaning there is a
path from `S1` to the end of the program that does not involve `S2`. This
fact will be leveraged when implmenting the construction of the CDG. A simple
way to find the basic blocks that are control dependent on some block `B1` is
to traverse all successive basic blocks of `B1` and mark them if they do
*not* post-dominate block `B1`.

Data Dependence
++++++++++++++++

Fundamentally, a data dependence exists between two instructions when one
instruction uses some piece of data that was modified by the other. The
following excerpt shows a naive data dependence between statements `S1` and
`S2`.

.. code-block:: language

    S1      x := 10 
    S2      y := x

More complicated data dependence relationships exist in languages like C and
C++ that contain pointers. For instance, a slightly more involved data
dependence exists between `S3` and `S4` in the following code:

.. code-block:: c

    S1      unsigned int * p = 0x12345678;
    S2      unsigned int * q = 0x12345678;
    S3      *p = 1;
    S4      read(q);

Due to potentially complicated memory aliasing relationships, a points-to
analysis is required to find some of these non-obvious dependencies. Our
implementation will heavily rely on the existing Data Dependence Graph in
LLVM-10. More information can be found here:
https://llvm.org/docs/DependenceGraphs/index.html.

Purpose
--------

Prior Work
-----------

Implementation
---------------