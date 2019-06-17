.. _example_visualize_local_lattice_orientation:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Example M3: Color mapping to visualize local lattice orientation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :ovitoman:`Polyhedredral Template Matching (PTM) <../../../../particles.modifiers.polyhedral_template_matching>` function of OVITO allows
computing the local lattice orientation for each atom in a (poly)crystal. The computed local orientations
are stored by the modifier as quaternions, i.e. as rotations within the fundamental zone, in the particle property named ``Orientation``.
Each per-particle quaternion can be translated into an RGB color to visualize the local lattice orientation.
This can be achieved by inserting a :ref:`custom Python modifier <writing_custom_modifiers>` into the pipeline which translates the output of the PTM modifier
into RGB values and stores them in the ``Color`` particle property.

In the graphical OVITO version, simply insert a new Python modifier and copy/paste the following script into the source code window:

.. literalinclude:: ../../../example_snippets/quaternions_to_colors.py
  :lines: 1-52