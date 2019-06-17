==================================
Code examples
==================================

This page presents various example scripts grouped into the following categories.

.. _batch_script_examples:

----------------------------------
Batch scripts
----------------------------------

As described in :ref:`scripting_running`, batch scripts perform program actions in a non-interactive manner
and are typically executed from the system terminal via the :program:`ovitos` script interpreter. The following examples demonstrate
how to automate tasks and accomplish new things that cannot even be done with the graphical program version.

.. toctree::
   :maxdepth: 1

   examples/batch_scripts/voronoi_indices
   examples/batch_scripts/cna_bond_indices
   examples/batch_scripts/creating_particles


.. _modifier_script_examples:

----------------------------------
User-defined modifier functions
----------------------------------

OVITO allows you to implement your :ref:`own type of analysis modifier <writing_custom_modifiers>` by writing a Python function that gets called every time
the data pipeline is evaluated. This user-defined function has access to the positions and other properties of particles
and can output information and results as new properties or global attributes.

.. toctree::
   :maxdepth: 1

   examples/modifiers/msd_calculation
   examples/modifiers/order_parameter_calculation
   examples/modifiers/visualize_local_lattice_orientation
   examples/modifiers/select_overlapping_particles

.. _overlay_script_examples:

----------------------------------------
User-defined overlay functions
----------------------------------------

OVITO allows you to implement your type of viewport overlay by writing a Python function that gets called every time
a viewport image is being rendered.

.. toctree::
   :maxdepth: 1

   examples/overlays/scale_bar
   examples/overlays/data_plot
   examples/overlays/highlight_particle

