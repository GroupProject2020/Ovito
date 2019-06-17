.. _example_order_parameter_calculation:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Example M2: Custom order parameter calculation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the paper `[Phys. Rev. Lett. 86, 5530] <https://doi.org/10.1103/PhysRevLett.86.5530>`__ an order parameter is specified as a means
of labeling an atom in the simulation as belonging to either the liquid or solid fcc crystal phase. In the following we will
develop a custom analysis modifier for OVITO, which calculates this per-atom order parameter.

The order parameter is defined as follows (see the paper for details): For any of the 12 nearest neighbors of a given atom one can compute the distance the neighbor
makes from the ideal fcc positions of the crystal in the given orientation (denoted by vector :strong:`r`:sub:`fcc`). The sum of the distances over the 12 neighbors,
phi = 1/12*sum(\| :strong:`r`:sub:`i` - :strong:`r`:sub:`fcc` \|), acts as an "order parameter" for the central atom.

Calculating this parameter involves finding the 12 nearest neighbors of each atom and, for each of these neighbors, determining the
closest ideal lattice vector. To find the neighbors, OVITO provides the :py:class:`~ovito.data.NearestNeighborFinder` utility class.
It directly provides the vectors from the central atom to its nearest neighbors.

Let us start by defining some inputs for the order parameter calculation at the global scope:

.. literalinclude:: ../../../example_snippets/order_parameter_calculation.py
  :lines: 9-34

The actual modifier function needs to create an output particle property, which will store the calculated
order parameter of each atom. Two nested loops run over all input atoms and their 12 nearest neighbors respectively.

.. literalinclude:: ../../../example_snippets/order_parameter_calculation.py
  :lines: 39-75

Note that the ``yield`` statements in the modifier function above are only needed to support progress feedback in the
graphical version of OVITO and to give the pipeline system the possibility to interrupt the long-running calculation when needed.
