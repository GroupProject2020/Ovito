==================================
Overview
==================================

The scripting interface gives you access to most of OVITO's program features. Using Python scripting, you can
do many things that are already familiar from the graphical user interface, and even some more:

  * :ref:`Import data from external files <file_io_overview>`
  * :ref:`Apply modifiers to a dataset and configure them <modifiers_overview>`
  * :ref:`Export data to a file <file_output_overview>`
  * :ref:`Set up a camera and render pictures or movies of the scene <rendering_viewports>`
  * :ref:`Control the visual appearance of particles and other objects <rendering_display_objects>`
  * :ref:`Access per-particle properties and other analysis results computed by OVITO <particle_properties_intro>`
  * :ref:`Implement new types of modifiers <writing_custom_modifiers>`

But before discussing these specific tasks, let's first take a look at some general concepts of OVITO's data model 
and scripting framework.

.. warning::
   This section of the manual is out of date! It has not been updated yet to reflect the changes made in the current
   development version of OVITO.

------------------------------------
OVITO's data pipeline architecture
------------------------------------

If you have used OVITO's graphical user interface before, you are already familiar with 
its central concept: the data processing pipeline. After loading a simulation file, you typically apply modifiers 
that act on the input data. The output of this sequence of modifiers (which form the *data pipeline*) is computed by OVITO 
and displayed in the interactive viewports. From a Python script's perspective, a data pipeline is represented
by an instance of the :py:class:`~ovito.pipeline.Pipeline` class, which stores the list of modifiers
in its :py:attr:`~ovito.pipeline.Pipeline.modifiers` field.

In addition to the modifier list, a data pipeline always has some kind of *data source*, which is an associated object providing 
the input data being passed to the pipeline's first modifier. Typically, the data source is an instance of the
:py:class:`~ovito.pipeline.FileSource` class. It is responsible for dynamically loading data from an external file.
OVITO also allows you to select other kinds of data sources for a pipeline, for example in case you want to 
directly specify the input data programmatically instead of loading it from file. 

A :py:class:`~ovito.pipeline.Pipeline` may be placed into the *scene*, i.e. the three-dimensional world that is visible
through OVITO's viewports and in rendered images. Only a :py:class:`~ovito.pipeline.Pipeline` that is part of the *scene*
will visually shows its computation results. You add a :py:class:`~ovito.pipeline.Pipeline` to the scene by calling its
:py:meth:`~ovito.pipeline.Pipeline.add_to_scene` method.

All pipeline objects currently part of the scene, and all other program state information that would get saved along in 
a :file:`.ovito` file (e.g. current render settings, viewport cameras, etc.), comprise a :py:class:`~ovito.DataSet`. 
Python scripts always run in the context of exactly one global :py:class:`~ovito.DataSet` instance. This 
instance is accessible through the :py:data:`ovito.dataset` global variable. 

------------------------------------
Loading data
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

------------------------------------
Applying modifiers
------------------------------------

We can now populate the data pipeline with modifiers by inserting them
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

OVITO's scripting interface allows you to directly access the output data leaving the
modification pipeline. But before doing so, we first have to ask OVITO to compute the results of the modification pipeline::

    >>> node.compute()
    
The :py:meth:`~ovito.ObjectNode.compute` method ensures that all modifiers in the pipeline of the node
have been successfully evaluated. Note that the :py:meth:`~ovito.vis.Viewport.render` and 
:py:func:`~ovito.io.export_file` functions implicitly call :py:meth:`~ovito.ObjectNode.compute`
for us. But now, since we want to directly access the pipeline results, we have to explicitly request 
an evaluation of the modification pipeline.

The node caches the results of the last pipeline evaluation in the :py:attr:`ObjectNode.output <ovito.ObjectNode.output>` field
in the form of a :py:class:`~ovito.data.DataCollection`::

    >>> node.output
    DataCollection(['Simulation cell', 'Particle Identifier', 'Position', 
                    'Potential Energy', 'Color', 'Structure Type'])
    
It contains all the *data objects* that were processed or produced  
by the modification pipeline. For example, to access the :py:class:`simulation cell <ovito.data.SimulationCell>` we would write::

    >>> node.output.cell.matrix
    [[ 148.147995      0.            0.          -74.0739975 ]
     [   0.          148.07200623    0.          -74.03600311]
     [   0.            0.          148.0756073   -74.03780365]]
     
    >>> node.output.cell.pbc
    (True, True, True)

Similarly, the data of individual :py:class:`particle properties <ovito.data.ParticleProperty>` may be accessed as NumPy arrays:

    >>> import numpy
    >>> node.output.particle_properties.position.array
    [[ 73.24230194  -5.77583981  -0.87618297]
     [-49.00170135 -35.47610092 -27.92519951]
     [-50.36349869 -39.02569962 -25.61310005]
     ..., 
     [ 42.71210098  59.44919968  38.6432991 ]
     [ 42.9917984   63.53770065  36.33330154]
     [ 44.17670059  61.49860001  37.5401001 ]]

See the :py:mod:`ovito.data` module for a list of data object types that may occur in a :py:class:`~ovito.data.DataCollection`.

Sometimes we might also be interested in the data that *enters* the modification pipeline.
The input data, which was read from the external file, is cached by the :py:class:`~ovito.io.FileSource`,
which is itself a :py:class:`~ovito.data.DataCollection`::

    >>> node.source
    DataCollection(['Simulation cell', 'Particle Identifier', 'Position'])

------------------------------------
Rendering images
------------------------------------

To render an image, we first need a viewport that defines the view on the three-dimensional scene.
We can either use one of the four predefined viewports of OVITO for this, or simply create an *ad hoc* 
:py:class:`~ovito.vis.Viewport` instance in Python::

    >>> from ovito.vis import *
    >>> vp = Viewport()
    >>> vp.type = Viewport.Type.PERSPECTIVE
    >>> vp.camera_pos = (-100, -150, 150)
    >>> vp.camera_dir = (2, 3, -3)
    >>> vp.fov = math.radians(60.0)
    
As you can see, the :py:class:`~ovito.vis.Viewport` class has several parameters that control the 
position and orientation of the camera, the projection type, and the field of view (FOV) angle. Note that this
viewport will not be visible in OVITO's main window, because it is not part of the current :py:class:`~ovito.DataSet`; 
it is only a temporary object used within the script.

In addition we need to create a :py:class:`~ovito.vis.RenderSettings` object, which controls the rendering
process (These are the parameters you normally set on the :guilabel:`Render` tab in OVITO's main window)::

    >>> settings = RenderSettings()
    >>> settings.filename = "myimage.png"
    >>> settings.size = (800, 600)
   
Now we have specified the output filename and the size of the image in pixels.
We should not forget to also add the :py:class:`~ovito.ObjectNode` to the *scene* by calling::

    >>> node.add_to_scene()

Because only object nodes that are part of the scene are visible in the viewports and in rendered images.
Finally, we can let OVITO render an image of the viewport::

    >>> vp.render(settings)
    
As a final remark, note how we could have used the more compact notation for object initialization introduced above.
We can configure the newly created :py:class:`~ovito.vis.Viewport` and :py:class:`~ovito.vis.RenderSettings` by passing the parameter values directly to the class constructors:: 

    vp = Viewport(
        type = Viewport.Type.PERSPECTIVE,
        camera_pos = (-100, -150, 150),
        camera_dir = (2, 3, -3),
        fov = math.radians(60.0)
    )
    vp.render(RenderSettings(filename = "myimage.png", size = (800, 600)))


-------------------------------------------------
Controlling the visual appearance of objects
-------------------------------------------------

So far we have only looked at objects that represent data, e.g. particle properties or the simulation cell. 
Let's see how this data is displayed and how we can control its visual appearance.

Every data object with a visual representation in OVITO is associated with a matching :py:class:`~ovito.vis.Display`
object. The display object is stored in the data object's :py:attr:`~.ovito.data.DataObject.display` property. For example::

    >>> cell = node.source.cell
    >>> cell                               # This is the SimulationCell data object
    <SimulationCell at 0x7f9a414c8060>
    
    >>> cell.display                       # This is its attached display object
    <SimulationCellDisplay at 0x7fc3650a1c20>

The :py:class:`~ovito.vis.SimulationCellDisplay` is responsible for rendering the simulation
cell in the viewports and provides parameters that allow us to configure the visual appearance. For example, to change the
display color of the simulation box::

    >>> cell.display.rendering_color = (1.0, 0.0, 1.0)

We can also turn off the display of any object entirely by setting the :py:attr:`~ovito.vis.Display.enabled`
attribute of the display to ``False``::

    >>> cell.display.enabled = False 

Particles are rendered by a :py:class:`~ovito.vis.ParticleDisplay` object. It is always attached to the 
:py:class:`~ovito.data.ParticleProperty` object storing the particle positions (which is the only mandatory particle
property that is always defined). Thus, to change the visual appearance of particles, 
we have to access the ``Positions`` particle property in the :py:class:`~ovito.data.DataCollection`::

    >>> pos_prop = node.source.particle_properties.position
    >>> pos_prop
    <ParticleProperty at 0x7ff5fc868b30>
      
    >>> pos_prop.display
    <ParticleDisplay at 0x7ff5fc868c40>
       
    >>> pos_prop.display.shading = ParticleDisplay.Shading.Flat
    >>> pos_prop.display.radius = 1.4
