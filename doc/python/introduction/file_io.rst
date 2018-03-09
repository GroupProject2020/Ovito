.. _file_io_overview:

===================================
File I/O
===================================

------------------------------------
Data import
------------------------------------

The standard way of loading external data from disk is calling the global :py:func:`~ovito.io.import_file` function::

   from ovito.io import import_file

   pipeline = import_file("simulation.dump")

This high-level function works similar to the `Load File` function in OVITO's graphical user interface and automatically detects the format of the input file. 
See the `OVITO user manual <../../usage.import.html#usage.import.formats>`__ for a list of supported file formats.
The function returns a new :py:class:`~ovito.pipeline.Pipeline` object, whose associated :py:class:`~ovito.pipeline.FileSource` is set up to point
to the specified data file. 

In case you already have an existing :py:class:`~ovito.pipeline.Pipeline` object, e.g. from a first call to :py:func:`~ovito.io.import_file`, 
it is possible to later switch the input file of the pipeline with a call to the :py:meth:`FileSource.load() <ovito.pipeline.FileSource.load>` method::

   pipeline.source.load("other_simulation.dump")

The :py:meth:`~ovito.pipeline.FileSource.load` method accepts the same parameters as the global :py:func:`~ovito.io.import_file` function, but it doesn't create a new
pipeline. Any modifiers in the existing pipeline are preserved, only its input data gets replaced with a different input file in this case.

.. note::

   Unlike the `Load File` function in OVITO's graphical user interface, the :py:func:`~ovito.io.import_file` function does *not*
   automatically insert the loaded data into the three-dimensional scene. That means the data will not, by default, appear in rendered images or 
   the interactive viewports (in case are running the graphical program). If you would like to see a visual representation of the imported data, you need to explicitly
   invoke :py:meth:`Pipeline.add_to_scene() <ovito.pipeline.Pipeline.add_to_scene>` to make the pipeline part of the current scene.
   More on this can be found in the :py:ref:`rendering_intro` section.

**Simulation trajectories**

The :py:class:`~ovito.pipeline.FileSource` class and the :py:func:`~ovito.io.import_file` function support loading sequences of simulation 
frames (*trajectories*) and make them available as time-dependent pipeline input. To load a series of individual simulation files that follow 
a naming pattern such as :file:`frame.0.dump`, :file:`frame.1000.dump`, :file:`frame.2000.dump`, etc., pass a wildcard pattern to 
the :py:func:`~ovito.io.import_file` function::

    pipeline = import_file("/path/frame.*.dump")

OVITO will automatically find all files in the directory matching the pattern and load them as one trajectory. File formats that
can store multiple frames are automatically loaded as trajectories by OVITO. You can check how many frames were found by querying the 
:py:attr:`FileSource.num_frames <ovito.pipeline.FileSource.num_frames>` property::

   print(pipeline.source.num_frames)

.. note::
   
   To save memory and time, OVITO does not load all frames of a trajectory at once. The call to :py:func:`~ovito.io.import_file` lets OVITO quickly scan the 
   directory or the multi-frame file to discover all frames belonging to the trajectory. 
   The actual data of a frame will only be loaded on demand, one at a time, whenever the pipeline is evaluated at a certain animation time, e.g., when jumping to a 
   new frame in the animation or when rendering a movie.

Some MD simulation codes store the topology of a molecular system (i.e. the definition of atom types, bonds, etc.) 
and the atomic trajectories in two separate files. In this case, load the topology file first using :py:func:`~ovito.io.import_file`. 
Then create and apply a :py:class:`~ovito.modifiers.LoadTrajectoryModifier`, which will load the time-dependent atomic positions from the 
separate trajectory file::

    pipeline = import_file("topology.data")
    traj_mod = LoadTrajectoryModifier()
    traj_mod.source.load('trajectory.dump')
    pipeline.modifiers.append(traj_mod)

**Accessing individual frames of a trajectory**

Once a simulation trajectory was loaded using :py:func:`~ovito.io.import_file`, we can step through the individual frames of the sequence using a ``for``-loop::

   for frame in range(pipeline.source.num_frames):
       data = pipeline.compute(frame)
       ...

The :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` method takes the frame number as argument at which the pipeline should
be evaluated. This call triggers a load operation and lets the pipeline's :py:class:`~ovito.pipeline.FileSource` fetch the requested input data from
the external simulation file. Note that frame numbering starts at 0 in OVITO.

**Column-to-property mapping**

When loading a MD simulation file, OVITO needs to map the stored information to corresponding particle properties in memory. Typically, this happens 
automatically. Certain file formats, however, do not contain sufficient information to perform this mapping automatically. For instance, when loading a 
plain XYZ file, the mapping of input file columns to OVITO's particle properties needs to be explicitly specified using the ``columns`` keyword::

   pipeline = import_file("simulation.xyz", columns = 
            ["Particle Type", "Position.X", "Position.Y", "Position.Z", "My Property"])
   
The number of entries in the ``columns`` list must match the number of data columns of the XYZ input file. 
See the documentation of the :py:func:`~ovito.io.import_file` function for more information on this.

.. _file_output_overview:

------------------------------------
Data export
------------------------------------

Exporting data to a file is typically done using the global :py:func:`ovito.io.export_file` function.
For example, to export the particle information that is output by a :py:class:`~ovito.pipeline.Pipeline` to a LAMMPS dump file, one would
write::

    from ovito.io import export_file

    export_file(pipeline, "outputfile.dump", "lammps/dump",
        columns = ["Position.X", "Position.Y", "Position.Z", "My Property"])

The second and third function parameters specify the output filename and the
file format. For a list of supported file formats, see the :py:func:`~ovito.io.export_file` documentation.
Depending on the selected format, additional keyword arguments may need to be specified. For instance
in the example above, the ``columns`` parameter specifies the list of particle properties to be exported to the output file.

In addition to particle data, :py:func:`~ovito.io.export_file` can also export other types of data computed by OVITO.
One example are *attributes*, which are global values output by modifiers during the pipeline evaluation.
In other words, unlike particle properties, attributes are computation results that are associated with the particle dataset as a whole.
For example, the :py:class:`~ovito.modifiers.ExpressionSelectionModifier` outputs an attribute with the name ``SelectExpression.num_selected``
to report the number of particles that matched the given selection criterion.

We can export the value of this computed attribute to a text file, typically for all frames of a simulation as a table, 
to graphically plot the time evolution using an external program. For this purpose the :py:func:`~ovito.io.export_file` function
supports the ``txt`` output format::

   pipeline = import_file("simulation*.dump")

   modifier = ExpressionSelectionModifier(expression = "PotentialEnergy < -3.9")
   pipeline.modifiers.append(modifier)

   export_file(pipeline, "potenergy.txt", "txt", multiple_frames = True,
            columns = ["Frame", "SelectExpression.num_selected"])

The ``multiple_frames`` keyword arguments tells the :py:func:`~ovito.io.export_file` function to evaluate the pipeline for all
animation frames. Without it, only the current frame (frame 0) would have been exported to the output file.
The program above produces a text file :file:`potenergy.txt` containing one line per simulation frame::

   0 531
   1 540
   2 522
   3 502
   ...

The first column contains the animation frame number (starting at 0) and the second
column contains the value of the ``SelectExpression.num_selected`` attribute generated by the :py:class:`~ovito.modifiers.ExpressionSelectionModifier`
in the pipeline.

Typically, global attributes are dynamically generated by modifiers in the pipeline, but some may also be loaded from the 
data file that serves as input for the data pipeline. For example, an attributed named ``Timestep`` is automatically generated by OVITO when importing a LAMMPS dump file,
denoting the simulation timestep number of the loaded snapshots. This makes it possible, for example,
to replace the animation frame number in the first column above (corresponding to the predefined attribute ``Frame``)
with the actual timestep number from the simulation. See :py:attr:`ovito.data.DataCollection.attributes` for more information.
