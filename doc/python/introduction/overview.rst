.. _scripting_api_overview:

==================================
Overview
==================================

The scripting interface gives you access to most of OVITO's program features. Using Python scripting, you can
do many things that are already familiar from the graphical user interface (and even some more):

  * Import data from simulation files
  * Compose modifiers and set up a data processing pipeline
  * Write computed data to an output file
  * Render images or movies of the three-dimensional scene and control the camera
  * Change the appearance of particles and other visual objects
  * Access data, e.g. particle properties, and analysis results computed by OVITO
  * Write your own data modification and analysis functions and extend OVITO's capabilities

But before discussing these specific topics, let's first take a look at some general concepts of OVITO's data model 
and scripting framework.

------------------------------------
OVITO's data pipeline architecture
------------------------------------

If you have used OVITO's graphical user interface before, you are already familiar with 
its central concept: the data processing pipeline. After loading a simulation file, you typically apply modifiers 
that act on the input data. The output of this sequence of modifiers (which form the *data pipeline*) is computed by OVITO 
and displayed in the interactive viewports. From a Python script's perspective, a data pipeline is represented
by an instance of the :py:class:`~ovito.pipeline.Pipeline` class, which stores the list of applied modifiers
in its :py:attr:`~ovito.pipeline.Pipeline.modifiers` field.

In addition to the modifier list, a data pipeline always has some kind of *data source*, which is an associated object providing 
the input data being passed to the pipeline's first modifier. Typically, the data source is an instance of the
:py:class:`~ovito.pipeline.FileSource` class. It is responsible for dynamically loading data from an external file.
OVITO also allows you to select other kinds of data sources for a pipeline, for example in case you want to 
directly specify the input data programmatically instead of loading it from file. 

.. image:: graphics/Pipeline_overview.*
   :width: 75 %
   :align: center

A :py:class:`~ovito.pipeline.Pipeline` may be placed into the *scene*, i.e. the three-dimensional world that is visible
through OVITO's viewports and in rendered images. Only a :py:class:`~ovito.pipeline.Pipeline` that is part of the *scene*
will visually shows its computation results. You add a :py:class:`~ovito.pipeline.Pipeline` to the scene by calling its
:py:meth:`~ovito.pipeline.Pipeline.add_to_scene` method.

All pipeline objects currently part of the scene, and all other program state information that would get saved along in 
a :file:`.ovito` file (e.g. current render settings, viewport cameras, etc.), comprise a :py:class:`~ovito.DataSet`. 
Python scripts always run in the context of exactly one global :py:class:`~ovito.DataSet` instance. This 
instance is accessible through the :py:data:`ovito.dataset` global variable. 

------------------------------------
Loading data from disk
------------------------------------

A new instance of the :py:class:`~ovito.pipeline.Pipeline` class is automatically created when you load a data file  
using the :py:func:`ovito.io.import_file` function::

   >>> from ovito.io import import_file
   >>> pipeline = import_file("simulation.dump")
   
This high-level function creates a :py:class:`~ovito.pipeline.Pipeline` (without modifiers yet) 
and wires it to a new :py:class:`~ovito.pipeline.FileSource` (which will subsequently load the data 
from the given file). The pipeline's data source is accessible through the :py:attr:`~ovito.pipeline.Pipeline.source`
property:: 

   >>> print(pipeline.source)
   <FileSource at 0x7f9ea70aefb0>

This allows you to later replace the pipeline's input data with a different external file if needed.
The :ref:`file_io_overview` section of this documentation provides more information on importing data into OVITO
and exporting it to output files.

------------------------------------
Applying modifiers
------------------------------------

We can now build up a processing pipeline by inserting modifiers
into the pipeline's :py:attr:`~ovito.pipeline.Pipeline.modifiers` list::

   >>> from ovito.modifiers import *
   >>> pipeline.modifiers.append(ColorCodingModifier(property = 'Potential Energy'))
   >>> pipeline.modifiers.append(SliceModifier(normal = (0,0,1)))

As shown in the example above, modifiers are constructed by invoking the constructor of one of the modifier classes, which are
all found in the :py:mod:`ovito.modifiers` module. Note how a modifier's parameters can be initialized in two different ways:

.. note::

   When constructing a new object (e.g. a modifier, but also most other OVITO objects) it is possible to directly initialize its
   properties by passing keyword arguments to the constructor function. Thus ::
   
       pipeline.modifiers.append(CommonNeighborAnalysisModifier(cutoff=3.2, only_selected=True))
       
   is equivalent to setting the properties one by one after constructing the object::

       modifier = CommonNeighborAnalysisModifier()
       modifier.cutoff = 3.2
       modifier.only_selected = True
       pipeline.modifiers.append(modifier)
   
   Obviously, the first way of initializing the object's parameters is more convenient and should be preferentially used
   whenever the parameter values are known at construction time. 

The :ref:`modifiers_overview` section of this documentation provides more information on working with modifiers and pipelines.

------------------------------------
Exporting data to a file
------------------------------------

Once a :py:class:`~ovito.pipeline.Pipeline` has been created, we can pass it to the :py:func:`ovito.io.export_file` function
to let OVITO compute the results of the pipeline and write them to an output file::

    >>> from ovito.io import export_file
    >>> export_file(pipeline, "outputdata.dump", "lammps/dump",
    ...    columns = ["Position.X", "Position.Y", "Position.Z", "Structure Type"])
    
In addition to the :py:class:`~ovito.pipeline.Pipeline` providing the output data, the :py:func:`~ovito.io.export_file` function
takes the output filename and the desired format as arguments. 
Furthermore, depending on the selected format, additional keyword arguments such as the list of particle properties to 
export must be provided. See the documentation of the :py:func:`~ovito.io.export_file` function and :ref:`this section <file_output_overview>`
of the manual for more information on the supported output formats and additional export options. 

------------------------------------
Accessing computation results
------------------------------------

We can explicitly request an evaluation of the data pipeline to obtain the computation results:

    >>> data = pipeline.compute()
    
The :py:meth:`~ovito.pipeline.Pipeline.compute` method make sure that the current input data was loaded and
all modifiers in the pipeline have been fully evaluated. It returns a :py:class:`~ovito.data.PipelineFlowState` object
containing the final results of the processing pipeline. A :py:class:`~ovito.data.PipelineFlowState` is a particular
form of :py:class:`~ovito.data.DataCollection`, which essentially is a heterogeneous container for *data objects*::

    >>> data.objects
    [SimulationCell(), ParticleProperty('Particle Identifier'), 
         ParticleProperty('Position'), ParticleProperty('Potential Energy'), 
         ParticleProperty('Color')]
    
In the example above, the data collection's :py:attr:`~ovito.data.DataCollection.objects` list contains one :py:class:`~ovito.data.SimulationCell` object and 
several :py:class:`~ovito.data.ParticleProperty` objects, some of which were loaded from the input file
and others that were dynamically computed/generated by modifiers in the processing pipeline.

The :py:attr:`~ovito.data.DataCollection.objects` list of the data collection stores the data objects in arbitrary order.
To access a particular kind of data object from the list, for example the :py:class:`~ovito.data.SimulationCell`, one typically uses the
:py:meth:`~ovito.data.DataCollection.expect` method, which looks up a data object of a particular type::

    >>> from ovito.data import SimulationCell
    >>> cell = data.expect(SimulationCell)
    >>> print(cell[...])
    [[ 148.147995      0.            0.          -74.0739975 ]
     [   0.          148.07200623    0.          -74.03600311]
     [   0.            0.          148.0756073   -74.03780365]]

All particle properties in the :py:class:`~ovito.data.DataCollection` are exposed by the :py:attr:`~ovito.data.DataCollection.particle_properties`
dictionary view, which allows accessing particle properties by name::

    >>> positions = data.particle_properties['Position']
    >>> positions
    ParticleProperty('Position')
    >>> position[...]
    [[ 73.24230194  -5.77583981  -0.87618297]
     [-49.00170135 -35.47610092 -27.92519951]
     [-50.36349869 -39.02569962 -25.61310005]
     ..., 
     [ 42.71210098  59.44919968  38.6432991 ]
     [ 42.9917984   63.53770065  36.33330154]
     [ 44.17670059  61.49860001  37.5401001 ]]

In addition :py:class:`~ovito.data.SimulationCell` and :py:class:`~ovito.data.ParticleProperty`, OVITO
knows several other types of data objects. See the :py:mod:`ovito.data` module for a list of data object types that may appear in a :py:class:`~ovito.data.DataCollection`.

The :ref:`particle_properties_intro` section in this documentation provides more information on this topic.

------------------------------------
Accessing the pipeline's input data
------------------------------------

In the preceding section we saw how the :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` allows us to 
access the output of the processing pipeline. Sometimes we are also interested in the unmodified data that *enters* the modification pipeline.
This input data, which is read from the external data file, is cached by the pipeline's :py:class:`~ovito.pipeline.FileSource`.
The :py:class:`~ovito.pipeline.FileSource` itself is a special form of :py:class:`~ovito.data.DataCollection` and provides the same 
programming interface for accessing the contained data objects::

    >>> input_data = pipeline.source
    >>> input_data.objects
    [SimulationCell(), ParticleProperty('Particle Identifier'), 
        ParticleProperty('Position'), ParticleProperty('Potential Energy')]

------------------------------------
Rendering images and movies
------------------------------------

As mentioned ealier, to visualize the data it is necessary to add the :py:class:`~ovito.pipeline.Pipeline` to the three-dimensional scene
first. This is done by invoking the :py:meth:`~ovito.pipeline.Pipeline.add_to_scene` method::

    >>> pipeline.add_to_scene() 

Furthermore, to render an image or a movie, a :py:class:`~ovito.vis.Viewport` instance is required, which defines the view on 
the three-dimensional scene. We can either use one of the four predefined viewports of OVITO for this, or create an *ad hoc* 
:py:class:`~ovito.vis.Viewport` instance as shown here::

    >>> from ovito.vis import *
    >>> vp = Viewport()
    >>> vp.type = Viewport.Type.PERSPECTIVE
    >>> vp.camera_pos = (-100, -150, 150)
    >>> vp.camera_dir = (2, 3, -3)
    >>> vp.fov = math.radians(60.0)
    
The :py:class:`~ovito.vis.Viewport`'s parameters control the position and orientation of the camera, the type of projection (perspective or parallel), 
and the field of view (FOV) angle. 

Finally, we need to set up a :py:class:`~ovito.vis.RenderSettings` data structure to specify the parameters of the rendering
process (e.g. image resolution, background color, output filename, render backend, etc). 

    >>> settings = RenderSettings()
    >>> settings.filename = "myimage.png"
    >>> settings.size = (800, 600)
   
The :py:meth:`Viewport.render() <ovito.vis.Viewport.render>` method starts the rendering process and returns after the final image 
has been saved under the output filename that was set in the :py:class:`~ovito.vis.RenderSettings` object::

    >>> vp.render(settings)
    
As a final remark, note how we could have used the more compact notation mentioned above for object initialization.
The newly created :py:class:`~ovito.vis.Viewport` and :py:class:`~ovito.vis.RenderSettings` object can be configured by passing 
the values directly to the class constructors:: 

    vp = Viewport(
        type = Viewport.Type.PERSPECTIVE,
        camera_pos = (-100, -150, 150),
        camera_dir = (2, 3, -3),
        fov = math.radians(60.0)
    )
    vp.render(RenderSettings(filename = "myimage.png", size = (800, 600)))

Please see the :ref:`rendering_intro` section in this documentation for more information on this topic.

------------------------------------
More details
------------------------------------

The following links lead to more in-depth information on various aspects of the OVITO scripting interface:

  * :ref:`file_io_overview`
  * :ref:`modifiers_overview`
  * :ref:`file_output_overview`
  * :ref:`rendering_viewports`
  * :ref:`rendering_display_objects`
  * :ref:`particle_properties_intro`
  * :ref:`writing_custom_modifiers`
