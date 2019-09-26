.. _scripting_api_overview:

==================================
Overview
==================================

The Python programming interface gives you programmatic access to most of OVITO's program features. Using Python scripts, you can
perform many of the things you know from the interactive user interface (and even more):

  * Import from and export data to simulation files
  * Use modifiers to set up a data processing pipeline
  * Render images or movies of the three-dimensional scene
  * Access result data computed by an analysis pipeline
  * Write your own data modification functions

------------------------------------
OVITO's data pipeline architecture
------------------------------------

If you have worked with OVITO's graphical user interface before, you should already be familiar with the
:ovitoman:`data pipeline <../../usage.modification_pipeline>` concept:
After loading a simulation file into the program, you typically build up a data pipeline by applying one or more modifiers
that act on the data in some way. The result of this sequence of modifiers
is dynamically computed and displayed in the interactive viewports of OVITO.

The Python interface of OVITO let's you construct data pipelines in the same way by setting up a sequence of
modifiers and configuring their parameters. The data pipeline is represented by an instance of the :py:class:`~ovito.pipeline.Pipeline` Python class, which manages
the list of applied modifiers. Furthermore, a data pipeline always has some type of *data source*, which is a separate object providing or generating
the input data that will be passed to the pipeline's first modifier. Typically, the data source is an instance of the
:py:class:`~ovito.pipeline.FileSource` class, which is responsible for loading the data from an external file on disk.

.. image:: graphics/Pipeline_overview.*
   :width: 75 %
   :align: center

If you want to visualize the results of a :py:class:`~ovito.pipeline.Pipeline`, you need to place the pipeline into
the *scene*, i.e. the three-dimensional space that is visible in rendered images.
Only pipelines that have explicitly been inserted into the current scene by calling the :py:meth:`~ovito.pipeline.Pipeline.add_to_scene` method
will show up in rendered images or in OVITO's interactive viewports.
Note that this happens automatically in the graphical user interface of OVITO, but you need to do it explicitly when using the
Python scripting interface.

The scene, including all explicitly inserted pipelines and other state information that would get saved along in a :file:`.ovito` file, is represented
by the :py:class:`~ovito.Scene` Python class. A script always runs in the context of one current :py:class:`~ovito.Scene` instance,
which is accessible through the :py:data:`ovito.scene` global variable.

------------------------------------
Importing data from disk
------------------------------------

You typically create a new :py:class:`~ovito.pipeline.Pipeline` by importing a simulation data file from disk
using the :py:func:`ovito.io.import_file` function::

   >>> from ovito.io import import_file
   >>> pipeline = import_file("simulation.dump")

This high-level function creates a new :py:class:`~ovito.pipeline.Pipeline` instance
and wires it to a :py:class:`~ovito.pipeline.FileSource`, which will take care of loading the data
from the given input file into memory. The :py:class:`~ovito.pipeline.FileSource` object is accessible through the pipeline's :py:attr:`~ovito.pipeline.Pipeline.source`
field::

   >>> print(pipeline.source)
   <FileSource at 0x7f9ea70aefb0>

The :py:class:`~ovito.pipeline.FileSource` may be reconfigured to load a different input file if desired,
allowing you to replace the original input of the pipeline with new data. This is useful if you intend to batch-process a
number of simulation files, reusing the same processing pipeline. The :ref:`file_io_overview` section gives
more details on how to import and export data using the scripting interface.

------------------------------------
Applying modifiers
------------------------------------

Initially, the :py:class:`~ovito.pipeline.Pipeline` created by the :py:func:`~ovito.io.import_file` function contains no modifiers.
That means it will simply pass through the original, unmodified data loaded by the :py:class:`~ovito.pipeline.FileSource` from disk.
We can change this by inserting some modifiers into the pipeline's :py:attr:`~ovito.pipeline.Pipeline.modifiers` list::

   >>> from ovito.modifiers import *
   >>> pipeline.modifiers.append(ColorCodingModifier(property = 'Potential Energy'))
   >>> pipeline.modifiers.append(SliceModifier(normal = (0,0,1)))

Modifiers are constructed by instantiating one of the built-in modifier classes, which are
all contained in the :py:mod:`ovito.modifiers` Python module. Note how a modifier's parameters can be initialized in two different ways:

.. note::

   When creating a new object such as an OVITO modifier, you can directly initialize its
   parameters by passing keyword arguments to the constructor function. Thus ::

       pipeline.modifiers.append(CommonNeighborAnalysisModifier(cutoff=3.2, only_selected=True))

   is equivalent to assigning the values to the parameter fields one by one after constructing the object::

       modifier = CommonNeighborAnalysisModifier()
       modifier.cutoff = 3.2
       modifier.only_selected = True
       pipeline.modifiers.append(modifier)

   Obviously, the first method of initializing the parameters is more convenient and is the recommended way
   whenever the parameter values are known at construction time of the object.

Keep in mind that it is possible to change the parameters of modifiers in a pipeline at any time, or to remove modifiers
from a pipeline again. This option is useful if you want to sequentially process the input data in multiple different
ways. Alternatively, you can also set up multiple data pipelines, all sharing the same data source or even some of the
modifiers. The :ref:`modifiers_overview` section provides more information on working with
data pipelines and modifiers.

------------------------------------
Exporting data to disk
------------------------------------

Once a :py:class:`~ovito.pipeline.Pipeline` has been set up, you can pass it to the :py:func:`ovito.io.export_file` function
to let OVITO compute the result of the pipeline and write it to an output file in the given format::

    >>> from ovito.io import export_file
    >>> export_file(pipeline, "outputdata.dump", "lammps/dump",
    ...    columns = ["Position.X", "Position.Y", "Position.Z", "Structure Type"])

The :py:func:`~ovito.io.export_file` function takes the output filename and the desired format as arguments, in addition
to the :py:class:`~ovito.pipeline.Pipeline` generating the data to be exported.
Furthermore, depending on the selected format, additional keyword arguments such as the list of particle properties to
export must be provided. See the documentation of the :py:func:`~ovito.io.export_file` function and :ref:`this section <file_output_overview>`
for more information.

------------------------------------
Accessing computation results
------------------------------------

Instead of directly piping the computation results to an output file, you can also request the pipeline
to return a :py:class:`~ovito.data.DataCollection` object, which represents the output data leaving the pipeline::

    >>> data = pipeline.compute()

The :py:meth:`~ovito.pipeline.Pipeline.compute` method performs two things: It first requests the input data from
the pipeline's data source. Then, it let's all modifiers of the pipeline act on the data, one by one. The final data state
is returned to the caller as a :py:class:`~ovito.data.DataCollection`, which essentially is a heterogeneous container storing
a set of *data objects* that each represent different parts of a dataset::

    >>> data.objects
    [SimulationCell(), Particles(), AttributeDataObject(), AttributeDataObject()]

In the example above, the :py:attr:`DataCollection.objects <ovito.data.DataCollection.objects>` list contains a :py:class:`~ovito.data.SimulationCell` object,
a :py:class:`~ovito.data.Particles` object and several global attribute objects, which were either loaded from the source data file
of the pipeline or which were generated on the fly by modifiers in the pipeline.

The :py:class:`~ovito.data.DataCollection` class provides various fields for accessing particular kinds of data objects,
for example the :py:attr:`~ovito.data.DataCollection.cell` field returns the :py:class:`~ovito.data.SimulationCell` object
storing the simulation cell vectors and position of the cell origin as a matrix::

    >>> print(data.cell[...])
    [[ 148.147995      0.            0.          -74.0739975 ]
     [   0.          148.07200623    0.          -74.03600311]
     [   0.            0.          148.0756073   -74.03780365]]

The :py:attr:`~ovito.data.DataCollection.particles` field returns the :py:class:`~ovito.data.Particles` container object, which
manages all particle properties::

    >>> print(data.particles.positions[...])
    [[ 73.24230194  -5.77583981  -0.87618297]
     [-49.00170135 -35.47610092 -27.92519951]
     [-50.36349869 -39.02569962 -25.61310005]
     ...,
     [ 42.71210098  59.44919968  38.6432991 ]
     [ 42.9917984   63.53770065  36.33330154]
     [ 44.17670059  61.49860001  37.5401001 ]]

The :ref:`particle_properties_intro` section provides more information on this topic.

------------------------------------
Accessing a pipeline's input data
------------------------------------

Sometimes it may be necessary to access the original data that *enters* a pipeline.
The input data is read from the input file by the pipeline's :py:class:`~ovito.pipeline.FileSource`.
This object also provides a :py:meth:`~ovito.pipeline.FileSource.compute` method returning a :py:class:`~ovito.data.DataCollection`::

    >>> input_data  = pipeline.source.compute()
    >>> output_data = pipeline.compute()

------------------------------------
Rendering images and movies
------------------------------------

As mentioned earlier, to visualize data it is necessary to add the :py:class:`~ovito.pipeline.Pipeline` to the three-dimensional scene.
This is done by invoking its :py:meth:`~ovito.pipeline.Pipeline.add_to_scene` method, typically right after creating a new pipeline::

    >>> pipeline = import_file("simulation.dump")
    >>> pipeline.add_to_scene()

Furthermore, to render an image or a movie, a :py:class:`~ovito.vis.Viewport` object is required, which defines the viewpoint from which
the scene is seen::

    >>> from ovito.vis import Viewport
    >>> vp = Viewport()
    >>> vp.type = Viewport.Type.Perspective
    >>> vp.camera_pos = (-100, -150, 150)
    >>> vp.camera_dir = (2, 3, -3)
    >>> vp.fov = math.radians(60.0)

The :py:class:`~ovito.vis.Viewport`'s parameters control the position and orientation of the virtual camera, the type of projection (perspective or parallel),
and the field of view (FOV) angle. To automatically position the camera such that all objects in the scene are fully visible, you may call
the viewport's :py:meth:`~ovito.vis.Viewport.zoom_all` method.
Finally, the :py:meth:`Viewport.render_image() <ovito.vis.Viewport.render_image>` method must be called to render an image and save it to disk::

    >>> vp.render_image(filename="myimage.png", size=(800,600))

Note that as part of the rendering process, all pipelines that have been inserted into the current scene will automatically be evaluated.

------------------------------------
Further reading
------------------------------------

The following sections provide more information on various aspects of OVITO's Python scripting interface:

  * :ref:`file_io_overview`
  * :ref:`modifiers_overview`
  * :ref:`particle_properties_intro`
  * :ref:`data_manipulation_intro`
  * :ref:`writing_custom_modifiers`
  * :ref:`rendering_intro`
