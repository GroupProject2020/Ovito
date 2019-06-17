.. _example_compute_cna_bond_indices:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Example B2: Computing CNA bond indices
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following script demonstrates how to use the :py:class:`~ovito.modifiers.CreateBondsModifier`
to create bonds between particles. The structural environment of each created bond
is then characterized with the help of the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier`,
which computes a triplet of indices for each bond from the topology of the surrounding bond network.
The script accesses the computed CNA bond indices in the output :py:class:`~ovito.data.DataCollection` of the
modification pipeline and exports them to a text file. The script enumerates the bonds of each particle
using the :py:class:`~ovito.data.BondsEnumerator` helper class.

The generated text file has the following format::

   Atom    CNA_pair_type:Number_of_such_pairs ...

   1       [4 2 1]:2  [4 2 2]:1 [5 4 3]:1
   2       ...
   ...

Python script:

.. literalinclude:: ../../../../../examples/scripts/cna_bond_analysis.py