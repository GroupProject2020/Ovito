.. _example_data_plot_overlay:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Example O2: Including data plots in rendered images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This user-defined viewport overlay function demonstrates how to use the `Matplotlib <https://matplotlib.org>`_ Python module to render the radial
distribution function, which is dynamically computed by a :py:class:`~ovito.modifiers.CoordinationAnalysisModifier`
in the data pipeline, on top the three-dimensional visualization.

.. image:: /../manual/images/viewport_overlays/python_script_plot_example.png

.. literalinclude:: ../../../example_snippets/overlay_data_plot.py
  :lines: 4-40