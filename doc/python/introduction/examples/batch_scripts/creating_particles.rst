.. _example_creating_particles_programmatically:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Example B3: Creating particles and bonds programmatically
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following script demonstrates the creation of particles, a simulation cell, and bonds on the fly
without loading them from an external simulation file. This approach can be used to implement custom data importers
or dynamically generate atomic structures within OVITO, which can then be further processed or exported to a file.

The script creates different data objects and adds them to a new :py:class:`~ovito.data.DataCollection`.
Finally, a :py:class:`~ovito.pipeline.Pipeline` is created and a :py:class:`~ovito.pipeline.StaticSource` object is used
to make the :py:class:`~ovito.data.DataCollection` its data source.

.. literalinclude:: ../../../example_snippets/create_new_particle_property.py
