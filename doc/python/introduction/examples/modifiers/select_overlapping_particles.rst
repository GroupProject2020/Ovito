.. _example_select_overlapping_particles:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Example M4: Finding overlapping particles
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows how to write a :ref:`user-defined modifier function <writing_custom_modifiers>` that searches for pairs of particles
whose distance of separation is within the specified cutoff distance. Then one of the two particles in the pair is selected by the modifier.
Subsequently, the user may apply the :py:class:`~ovito.modifiers.DeleteSelectedModifier` to remove these selected particles from the system
and eliminate any potential overlaps among particles.

The modifier function below makes use of the :py:class:`~ovito.data.CutoffNeighborFinder` utility class, which allows finding
neighboring particles that are within a certain range of a central particles. The modifier produces the standard output particle property
``Selection``.

.. literalinclude:: ../../../example_snippets/select_close_particles.py
  :lines: 8-42