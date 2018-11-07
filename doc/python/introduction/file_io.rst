.. _file_io_overview:

===================================
File I/O
===================================

------------------------------------
Data import
------------------------------------

The most common way of loading simulation data from a file on disk is calling the global :py:func:`~ovito.io.import_file` function::

   from ovito.io import import_file

   pipeline = import_file("simulation.dump")

This high-level function works similar to the `Load File` function in OVITO's graphical user interface and automatically detects the format of the specified file. 
See the `OVITO user manual <../../usage.import.html#usage.import.formats>`__ for a list of supported file formats.
The function returns a new :py:class:`~ovito.pipeline.Pipeline` object, with an associated :py:class:`~ovito.pipeline.FileSource` set up to 
load the specified data file. 

In case you would like to re-use an existing :py:class:`~ovito.pipeline.Pipeline`, returned by an earlier call to :py:func:`~ovito.io.import_file`, 
it is possible to subsequently switch the input file with a call to the :py:meth:`FileSource.load() <ovito.pipeline.FileSource.load>` method::

   pipeline.source.load("other_simulation.dump")

The :py:meth:`~ovito.pipeline.FileSource.load` method accepts the exact same parameters as the global :py:func:`~ovito.io.import_file` function but won't create a whole new
pipeline. Any modifiers of the existing pipeline will be preserved, only its input gets replaced with the data from a different file.

.. note::

   Unlike the `Load File` function in OVITO's graphical user interface, the :py:func:`~ovito.io.import_file` function does *not*
   automatically insert the loaded data into the three-dimensional scene. That means the data will not, by default, appear in rendered images or 
   the interactive viewports (in case are running the graphical program). If you would like to see a visual representation of the imported data, you need to explicitly
   invoke :py:meth:`Pipeline.add_to_scene() <ovito.pipeline.Pipeline.add_to_scene>` to make the pipeline part of the current scene.
   More on this can be found in the :py:ref:`rendering_intro` section.

**Loading simulation trajectories**

The :py:class:`~ovito.pipeline.FileSource` class and the :py:func:`~ovito.io.import_file` function support loading a sequence of snapshots 
(i.e. a simulation trajectory). This happens automatically if the frames are all stored in one input file. Many simulation codes, however, 
produce a series of data files containing one frame each. To load such a file series, which follows a naming pattern such as :file:`frame.0.dump`, :file:`frame.1000.dump`, :file:`frame.2000.dump`, etc., pass a wildcard pattern to 
the :py:func:`~ovito.io.import_file` function::

    pipeline = import_file("/path/frame.*.dump")

OVITO automatically finds all files matching the pattern (must all be in one directory) and loads them as one trajectory. The third option
is to specify the list of files explicitly::

    file_list = [
        "dir_a/simulation.dump", 
        "dir_b/simulation.dump",
        "dir_c/simulation.dump"
    ]
    pipeline = import_file(file_list)

The :py:attr:`FileSource.num_frames <ovito.pipeline.FileSource.num_frames>` property tells you how many frames are in the loaded simulation trajectory::

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

In the loop, the :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` method is called with the frame number as argument at which the pipeline should
be evaluated. As part of this :py:meth:`~ovito.pipeline.Pipeline.compute` call, the pipeline's :py:class:`~ovito.pipeline.FileSource` will fetch the input data 
of the requested frame from the external simulation file(s). Note that frame numbering starts at 0 in OVITO.

**File column to property mapping**

When loading a simulation file containing atoms or other types of particles, OVITO needs to map the stored per-particle information to corresponding 
`particle properties  <../../usage.particle_properties.html>`__ within OVITO's internal data model. Typically, this mapping happens automatically. 
Certain file formats, however, do not contain sufficient information to perform it automatically. For instance, when loading a legacy
XYZ file, which can contain any number of file columns with user-defined meanings, the mapping of these file columns to OVITO's particle properties needs 
to be explicitly specified using the ``columns`` keyword::

   pipeline = import_file("simulation.xyz", columns = 
            ["Particle Type", "Position.X", "Position.Y", "Position.Z", "My Property"])
   
The number of entries in the ``columns`` list must match the number of data columns of the XYZ input file. 
See the documentation of the :py:func:`~ovito.io.import_file` function for more information on this.

.. _file_output_overview:

------------------------------------
Data export
------------------------------------

Exporting data to an output file is typically done using the global :py:func:`ovito.io.export_file` function.
For example, to export the particles and their properties, some of which may have been computed by a :py:class:`~ovito.pipeline.Pipeline`, 
one would write::

    from ovito.io import export_file

    export_file(pipeline, "outputfile.dump", "lammps/dump",
        columns = ["Position.X", "Position.Y", "Position.Z", "My Property"])

The second and third function parameters specify the output filename and the
file format. For a list of supported file formats, see the :py:func:`~ovito.io.export_file` documentation.
Depending on the selected format, additional keyword arguments may need to be specified. For instance,
in the example above, the ``columns`` parameter specifies the list of particle properties to be exported to the output file.

In addition to particles, :py:func:`~ovito.io.export_file` can also export other types of data computed by OVITO.
One example are *attributes*, which are global quantities computed by modifiers in a pipeline.
In other words, attributes are global information that is associated with the dataset as a whole.
For example, the :py:class:`~ovito.modifiers.ExpressionSelectionModifier` outputs the ``SelectExpression.num_selected`` attribute
to report the number of particles that matched the given selection criterion.

We can export the value of this dynamically computed attribute to a text file, typically for all frames of a trajectory as a table.
Such a table could then be used to produce a chart of the time evolution of the quantity using an external plotting program. 
For this purpose the :py:func:`~ovito.io.export_file` function supports the ``txt/attr`` output format::

   pipeline = import_file("simulation*.dump")

   modifier = ExpressionSelectionModifier(expression = "PotentialEnergy < -3.9")
   pipeline.modifiers.append(modifier)

   export_file(pipeline, "potenergy.txt", "txt/attr", multiple_frames = True,
            columns = ["Frame", "SelectExpression.num_selected"])

The ``multiple_frames`` option tells :py:func:`~ovito.io.export_file` to evaluate the pipeline for all
frames of the loaded trajectory. Without it, only the first frame (frame 0) would have been exported to the output file.
The program above produces a text file containing one line per animation frame::

   # "Frame" "SelectExpression.num_selected"
   0 531
   1 540
   2 522
   3 502
   ...

The first column contains the animation frame number (starting at 0) and the second
column contains the value of the ``SelectExpression.num_selected`` attribute calculated by the 
:py:class:`~ovito.modifiers.ExpressionSelectionModifier` as part of the data pipeline.

Typically, global attributes are dynamically computed by modifiers in the pipeline, but some may also be directly read from the  
input data file. For example, an attributed named ``Timestep`` is automatically generated by OVITO when importing a LAMMPS dump file,
reporting the simulation timestep number at the current animation frame. This makes it possible, for example,
to replace the animation frame number in the first file column above
with the actual timestep of the MD simulation. See :py:attr:`ovito.data.DataCollection.attributes` for more information on global attributes.
