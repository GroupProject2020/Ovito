.. _file_io_overview:

===================================
File I/O
===================================

This section describes how to load simulation data from external files and how to :ref:`export data <file_output_overview>`
computed by OVITO to a file again.

------------------------------------
Data import
------------------------------------

The standard way of loading external data is calling the global :py:func:`~ovito.io.import_file` function::

   >>> from ovito.io import import_file
   >>> node = import_file("simulation.dump")

This high-level function works similar to the `Load File` function in OVITO's graphical user interface. 
It creates and returns an :py:class:`~ovito.ObjectNode`, whose :py:class:`~ovito.io.FileSource` is set up to point
to the specified file and loads it.

In case you already have an existing :py:class:`~ovito.ObjectNode`, for example after a first call to :py:func:`~ovito.io.import_file`, 
you can subsequently load different simulation files by calling the :py:meth:`~ovito.io.FileSource.load` method
of the :py:class:`~ovito.io.FileSource` owned by the node::

   >>> node.source.load("next_simulation.dump")

This method takes the same parameters as the :py:func:`~ovito.io.import_file` global function, but it doesn't create a new
object node. Any existing modifiers assigned to the object node are preserved, only the input data is replaced.

Note that the same :py:meth:`~ovito.io.FileSource.load` method is also used when
loading reference configurations for analysis modifiers that require reference particle coordinates, e.g.::

   >>> modifier = CalculateDisplacementsModifier()
   >>> modifier.reference.load("reference.dump")

Here the :py:attr:`~ovito.modifiers.CalculateDisplacementsModifier.reference` attribute refers 
to a second :py:class:`~ovito.io.FileSource`, which is owned by the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` and which is responsible
for loading the reference particle positions required for the displacement vector calculation.

**Specifying the column-to-property mapping**

OVITO automatically detects the format of input files, but both the global :py:func:`~ovito.io.import_file` function and the 
:py:meth:`FileSource.load() <ovito.io.FileSource.load>` method accept format-specific keyword arguments that further control the import process. 
For instance, when loading XYZ
files, the mapping of input file columns to OVITO's particle properties needs to be specified using the ``columns`` keyword::

   >>> node = import_file("simulation.xyz", columns = 
   ...         ["Particle Type", "Position.X", "Position.Y", "Position.Z", "My Property"])
   
The number of entries in the ``columns`` list must match the number of data columns present in the XYZ input file. 
See the documentation of the :py:func:`~ovito.io.import_file` function for more information on this.

**Simulation sequences**

So far we only considered loading single simulation snapshots. As you know from the graphical program, OVITO is also able to
load sequences of simulation snapshots (trajectories), which can be played back as animations.
There are two scenarios:

1. To load a file containing multiple simulation frames, use the ``multiple_frames`` keyword::

    >>> node = import_file("sequence.dump", multiple_frames = True)

   OVITO will scan the entire file and discover all contained simulation frames. This works for LAMMPS dump files and XYZ files, for example.   

2. To load a series of simulation files from a directory, following a naming pattern like :file:`frame.0.dump`, :file:`frame.1000.dump`,
   :file:`frame.2000.dump`, etc., pass a wildcard pattern to the :py:func:`~ovito.io.import_file` function::

    >>> node = import_file("frame.*.dump")

   OVITO will automatically find all files in the directory belonging to the simulation trajectory.

In both cases you can check how many frames were found by querying the :py:attr:`~ovito.io.FileSource.num_frames` property 
of the :py:class:`~ovito.io.FileSource`, e.g.::

   >>> node.source.num_frames
   85

.. note::
   
   To save memory and time, OVITO never loads all frames of a trajectory at once. It only scans the directory (or the multi-frame file) 
   to discover all frames belonging to the sequence and adjusts the internal animation length to match the number of input frames found. 
   The actual simulation data of a frame will only be loaded by the :py:class:`~ovito.io.FileSource` on demand, e.g., when 
   jumping to a specific frame in the animation or when rendering a movie.
   
You can loop over the frames of a loaded animation sequence::

   # Load a sequence of simulation files 'frame0.dump', 'frame1000.dump', etc.
   node = import_file("simulation*.dump")

   # Set up data pipeline, apply modifiers as needed, e.g.
   node.modifiers.append(CoordinationNumberModifier(cutoff=3.2))
   
   for frame in range(node.source.num_frames):

       # This loads the input data for the current frame and 
       # evaluates the applied modifiers:
       output = node.compute(frame)

       # Work with the computation results
       ...


.. _file_output_overview:

------------------------------------
Data export
------------------------------------

Exporting particles and other computation results to a file is typically done using the global :py:func:`ovito.io.export_file` function.
For example, to export the particles that leave the modification pipeline of an :py:class:`~ovito.ObjectNode` to a LAMMPS dump file, one would
write::

    >>> export_file(node, "outputfile.dump", "lammps_dump",
    ...    columns = ["Position.X", "Position.Y", "Position.Z", "My Property"])

OVITO automatically evaluates the node's modification pipeline to obtain the computation results and writes them to the file.
Of course, if the node's modification pipeline contains no modifiers, then the original data loaded via :py:func:`~ovito.io.import_file` is exported. 

The second function parameter specifies the output filename, and the third parameter selects the 
output format. For a list of supported file formats, see the :py:func:`~ovito.io.export_file` documentation.
Depending on the selected output format, additional keyword arguments may need to be specified. For instance,
in the example above the ``columns`` parameter specifies the list of particle properties to be exported.

In addition to particles, :py:func:`~ovito.io.export_file` can also export other types of data computed by OVITO.
One example are global attributes, which are data values generated by modifiers during the pipeline evaluation.
In other words, unlike particle properties, attributes are computation results that are associated with a particle dataset as a whole.
For example, the :py:class:`~ovito.modifiers.SelectExpressionModifier` outputs an attribute with the name ``SelectExpression.num_selected``
to report the number of particles that matched the given selection criterion.

You can export the value of this computed attribute to a text file, typically for all frames of a simulation as a table, 
to graphically plot the time evolution using an external program. For this purpose the :py:func:`~ovito.io.export_file` function
supports the ``txt`` output format::

   node = import_file("simulation*.dump")

   node.modifiers.append(SelectExpressionModifier(expression="PotentialEnergy<-3.9"))

   export_file(node, "potenergy.txt", "txt", multiple_frames=True,
            columns = ["Frame", "SelectExpression.num_selected"])

This produces a text file :file:`potenergy.txt` containing one line per simulation frame and two columns::

   0 531
   1 540
   2 522
   3 502
   ...

The first column is the animation frame number (starting at 0) and the second
column contains the value of the ``SelectExpression.num_selected`` attribute output by the :py:class:`~ovito.modifiers.SelectExpressionModifier`.

Typically, attributes are generated by modifiers in the pipeline, but some may also be defined
during file import. For example, an attributed named ``Timestep`` is set by OVITO when importing a LAMMPS dump file,
which specifies the simulation timestep number of the loaded snapshots. This makes it possible, for example,
to replace the animation frame number in the first column above (corresponding to the predefined attribute ``Frame``)
with the actual timestep number from the simulation. See :py:attr:`ovito.data.DataCollection.attributes` for more information.
