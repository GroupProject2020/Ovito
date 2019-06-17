.. _example_msd_calculation:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Example M1: Calculating mean square displacement
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example presents a user-defined modifier function for calculating the mean square displacement (MSD) for a system of moving particles.
OVITO provides the built-in :ovitoman:`Displacement Vectors <../../../../particles.modifiers.displacement_vectors>` modifier, which
calculates the individual displacement of each particle. It stores its results in the ``"Displacement Magnitude"``
particle property. So all our user-defined modifier function needs to do is to sum up the squared displacement magnitudes and divide by the number of particles:

.. literalinclude:: ../../../example_snippets/msd_calculation.py
  :lines: 12-23

When used within the graphical program, the MSD value computed by this custom modifier may be exported to a text file as a function of simulation time using
OVITO's standard file export feature (Select ``Table of Values`` as output format).

Alternatively, we can make use of the custom modifier function from within a non-interactive batch script, which is run
with the ``ovitos`` interpreter. Then we have to insert the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` programmatically:

.. literalinclude:: ../../../example_snippets/msd_calculation.py