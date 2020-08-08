====================================
Program Dependence Graphs in PIRATE
====================================

Introduction
-------------

A Program Dependence Graph (PDG) is a *program representation* that conveys
both data and control dependencies. In a PDG, nodes are usually instructions
and edges between these nodes indicate some form of dependence. Edges can
represent a data dependence, control dependence, or call dependence. A PDG
can be built by building the Control Dependence Graph (CDG) and Dependence
Graph (DDG) independently, and then combining the results. The CDG captures
the control dependencies within the program while the DDG captures the data
dependencies. Recent research has leveraged PDGs to aid in
*program slicing*, which is the process of "slicing" an existing (usually
monolithic) program into separate partitions. Slices of a program can be
identified by disjoint partitions in the PDG. If a partition *A* of a PDG is
disjoint from another partition *B*, then the instructions contained within
should not have any control or data dependencies. This means they can run
orthogonally, or alternatively phrased, in their own PIRATE enclaves. Program
Dependence Graphs have other applications, such as finding program
dependencies for the sake of parallelization or optimization of code.

Control Dependence Graph
+++++++++++++++++++++++++

The following code exceprt shows a control dependence between statement
``S1`` and statement ``S2``. Statement ``S2`` is *control dependent* on
statement ``S1`` because ``S2`` will only execute if ``S1`` is false.

.. code-block:: language

    S1      if x > 2 goto L1 
    S2      y := 3 
    S3  L1: z := y + 1
    
Alternately phrased, ``S1`` is *not* post-dominated by ``S2``, meaning there
is a path from ``S1`` to the end of the program that does not involve ``S2``.
This fact will be leveraged when implmenting the construction of the CDG. A
simple way to find the basic blocks that are control dependent on some block
``B1`` is to traverse all successive basic blocks of ``B1`` and mark them if
they do
*not* post-dominate block ``B1``.
Though control flow dependencies can be expressed at the basic block
granularity, the nodes of the CDG will be instructions. This is because the
resulting PDG will have instructions as nodes, and control flow queries will
likely be made over instructions rather than basic blocks. Any algorithm used
for constructing a CDG using basic blocks can be trivially extended to use
instructions instead. The process simply involves adding edges from the
terminator instruction (typically an if-condition predicate) of the "from"
block to all instructions in the "to" block.

An edge in the CDG from vertex *u* to vertex *v* indicates that *u*
**influences** *v*, or that *v* is control dependent on *u*. As an example, a
segment of code and the corresponding CDG are provided below:

.. code-block:: c

    S1      x = 10
    S2      if (x == 10)
    S3          x = x + 1;
    S4      else 
    S5          x = x - 1;

.. image:: cdg_example.png
    :align: center

Data Dependence Graph
++++++++++++++++++++++

Fundamentally, a data dependence exists between two instructions when one
instruction uses some piece of data that was modified by the other. The
following excerpt shows a naive data dependence between statements ``S1`` and
``S2``.

.. code-block:: language

    S1      x := 10 
    S2      y := x

More complicated data dependence relationships exist in languages that
contain pointers (C and C++ for example). For instance, a slightly more
involved data dependence exists between ``S3`` and ``S4`` in the following
code:

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

Within the context of PIRATE, Program Dependence Graphs will aid in the task
of *intraprocedural conflict identification*. Conflicts are defined as
program points that contain dependencies (data or control) from *more than
one* domain. As a motivating example, we can think of two
different domains: orange and green. Each of these domains has a different
responsibility. The orange domain could be responsible for retrieving GPS
coordinates and the green domain could be responsible for filtering them. A
monolithic application that achieves this task could look like the following:

.. code-block:: c

    S1      GPS * gps = new GPS();           __pirate_enclave(orange)
    S2      Filter * filter = new Filter();  __pirate_enclave(green)
    S3      gps->get_coords();
    S4      filter->redact(gps);

Here, the ``redact`` method takes two parameters: a reference to a ``Filter``
object and a reference to a ``gps`` object. Since these two objects originate
from different enclaves (as marked by the ``__pirate_enclave`` attributes) a
conflict exists. This conflict can be found by using a Program Dependence
Graph. The following diagram shows the Program Dependence Graph of the above
program:

.. image:: conflict.png
    :align: center

The data dependencies are marked in the colors corresponding to the
originating enclave. There aren't any control dependencies because the
program has a flat structure. ``S4`` is data-dependent on ``S1`` because of
the ``gps`` parameter and is data-dependent on ``S2`` because of the hidden
``this`` parameter (``filter``). In order to find conflicts the enclave
annotations need to be
*propagated* to the correspondingly dependent nodes. In this example, both
the orange and green domains are propagated to ``S4`` from ``S1`` and ``S2``
respectively. This is done by following the data dependency edges in the PDG
accordingly.

Because a conflict was identified in the above program, it is not trivially
partitionable. The call to ``filter`` will need to be translated to an
Interprocess Communication (IPC) call, so that the two domains (enclaves) can
run orthogonally. The strategy for performing the domain isolation is
separate from the use of the PDG for *identifying* conflicts, so it will not
be covered in this document.

Implementation
---------------

Currently, LLVM-10 has an implementation for a Data Dependence Graph, but not
for a Control Dependence Graph or a Program Dependence Graph. Thus, the
current analysis infrastructure needs to be extended to ultimately support
the Program Dependence Graph. As of now, there are two options that should be
discussed for achieving this goal.

Option #1
+++++++++++

The current LLVM documentation
(https://llvm.org/docs/DependenceGraphs/index.html) shows an architecture
where the ``ProgramDependenceGraph`` is built using the builder design
pattern using the pre-existing ``DependenceGraphBuilder`` class. The
structure of this class can be seen here:
https://www.llvm.org/doxygen/classllvm_1_1AbstractDependenceGraphBuilder.html#details.
The important function of this class is ``populate()``, which makes calls to
the dependence graph construction algorithm. This function currently calls
both ``createDefUseEdges()`` and ``createMemoryDependencyEdges()`` which are
great for data dependency graphs, but do not make any reference to adding
control edges needed for the CDG and PDG. Thus, option #1 will involve
extending this function to include a ``createControlEdges()`` function.

.. image:: controledges
    :align: center

This function would implement the post-dominanator tree algorithm mentioned
in the *Control Dependence Graph* section.

Ultimately, this option **will not involve** a separate
``ControlDependenceGraph`` class; instead, the UML diagram outlined in
https://llvm.org/docs/DependenceGraphs/index.html will be obeyed. A
``ProgramDependenceGraph`` class will be created, along with a ``PDGBuilder``
class that creates *both* data and control edges. One important caveat is
that the Program Dependence Graph should be *interprocedural* in the end.
This means a separate pass will need to run over the LLVM module to connect
function call sites to their constructed PDG. The strategy for linking call
sites with the formal PDG is still under development.

Option #2
++++++++++

The second option involves a slightly different class architecture than
option #1. Instead of the UML diagram shown above in the official LLVM
documentation (where only ``ProgramDependenceGraph`` and
``DataDependenceGraph`` exist) this option will implement
``ControlDependenceGraph`` using the current
builder pattern. The UML diagram for this option will look something like the
following:

.. image:: option2_uml.png
    :align: center

This option differs from the first in that the ``ProgramDependenceGraph``
would *not* be built using the current builder pattern. Only the CDG and DDG
would be built using ``CDGBuilder`` and ``DDGBuilder`` respectively. The
``ProgramDependenceGraph`` class would simply be constructed using an
inter-procedural analysis pass that builds the CDG and DDG for each function,
and then constructs its own graph based on the union of the two. Importantly,
the current ``DependenceGraphBuilder`` in LLVM (linked above) does not seem
to support this paradigm well, as it looks like all graphs build using the
``DependenceGraphBuilder`` class were intended to have data dependencies.

References
-----------
