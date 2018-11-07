.. _rendering_intro:

===================================
Rendering & visualization
===================================

.. warning::
   This section of the manual is out of date! It has not been updated yet to reflect the changes made in the current
   development version of OVITO.


.. _rendering_display_objects:

-----------------------------------
Visual elements
-----------------------------------

In OVITO, *data objects* are separated from *visual elements*, which are responsible for
producing a visual representation of the data object. For example, a :py:class:`~ovito.data.SimulationCell` 
is a data object storing the simulation cell vectors and the periodic boundary flags. 
The corresponding visual element,  a :py:class:`~ovito.vis.SimulationCellVis`,
takes this information to generate the actual geometry primitives to visualize the simulation
cell in the viewports and in rendered pictures. A visual element typically has a set of parameters that 
control the visual appearance, for example the line color of the simulation box.

A visual element is attached to the data object that is should visualize, and you access it through the :py:attr:`~ovito.data.DataObject.vis`
attribute of the :py:class:`~ovito.data.DataObject` base class::

    >>> data.cell                                # This is the data object
    <SimulationCell at 0x7f9a414c8060>
    
    >>> data.cell.vis                            # This is the attached visual element
    <SimulationCellVis at 0x7fc3650a1c20>

    >>> data.cell.vis.rendering_color = (1,0,0)  # Giving the simulation box a red color
    
All display objects are derived from the :py:class:`~ovito.vis.DataVis` base class, which defines
the :py:attr:`~ovito.vis.DataVis.enabled` attribute that turns the display on or off::

    >>> data.cell.vis.enabled = False         # This hides the simulation cell
    
The visual display of particles is controlled by a :py:class:`~ovito.vis.ParticlesVis` object, which
is attached to the :py:class:`~ovito.data.Particles` data object. For example, to display 
cubic particles::

    >>> data.particles.vis.shape = ParticlesVis.Shape.Square

Some modifiers in a data pipeline may produce new data objects, for example as a result of a computation.
The :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` generates a new particle :py:class:`~ovito.data.Property` 
that stores the computed displacement vectors. To support the visualization of displacement vectors
as arrows, the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` automatically attaches a
:py:class:`~ovito.vis.VectorVis` element to the new particle property. We can access this visual element
in two ways: either directly through the :py:attr:`~ovito.modifiers.CalculateDisplacementsModifier.vector_vis` field of the modifier::

    >>> modifier = CalculateDisplacementsModifier()
    >>> pipeline.modifiers.append(modifier)
    >>> modifier.vector_vis.enabled = True       # Enable the display of arrows
    >>> modifier.vector_vis.color = (0,0,1)      # Give arrows a blue color

or via the :py:attr:`~ovito.data.DataObject.vis` field of the particle :py:class:`~ovito.data.Property` ::

    >>> data = pipeline.compute()                                  
    >>> data.particles.displacements.vis.enabled = True     # Enable the display of arrows
    >>> data.particles.displacements.vis.color = (0,0,1)    # Give arrows a blue color
    
.. _rendering_viewports:

-----------------------------------
Viewports
-----------------------------------

A :py:class:`~ovito.vis.Viewport` defines a view of the three-dimensional scene, in which the visual representation of the data
of a pipeline is generated. To render a picture of the scene, you typically create a new *ad hoc* :py:class:`~ovito.vis.Viewport` 
object and configure it by setting the camera position and orientation::

    >>> from ovito.vis import Viewport
    >>> vp = Viewport()
    >>> vp.type = Viewport.Type.Perspective
    >>> vp.camera_pos = (-100, -150, 150)
    >>> vp.camera_dir = (2, 3, -3)
    >>> vp.fov = math.radians(60.0)

As known from the interactive OVITO program, there exist various standard viewport types such as ``TOP``, ``FRONT``, etc. 
The ``PERSPECTIVE`` and ``ORTHO`` viewport types allow you to freely orient the camera in space and
are usually what you need in a Python script. Don't forget to set the viewport type first before configuring any other camera-related parameters. 
That's because changing the viewport type will reset the camera orientation to a default value.

The ``PERSPECTIVE`` viewport type selects a perspective projection, and you can control the vertical field of view 
by setting the :py:attr:`~ovito.vis.Viewport.fov` parameter to the desired angle. The ``ORTHO`` viewport type
uses a parallel projection; In this case, the :py:attr:`~ovito.vis.Viewport.fov` parameter specifies the vertical size of the visible
area in units of length. Optionally, you can call the :py:meth:`Viewport.zoom_all() <ovito.vis.Viewport.zoom_all>`
method to let OVITO automatically choose a reasonable camera zoom and position such that all objects become completely visible.

-----------------------------------
Rendering
-----------------------------------

Parameters that control the rendering process, e.g. the desired image resolution, output filename, background color, are managed by a 
:py:class:`~ovito.vis.RenderSettings` objects. You can create a new instance of this class and specify 
the parameters::

    from ovito.vis import *
    settings = RenderSettings(
        filename = "myimage.png",
        size = (800, 600)
    )

You can choose between three different rendering engines, which can produce the final image
of the scene. The default renderer is the :py:class:`~ovito.vis.OpenGLRenderer`, which implements a fast, hardware-accelerated
OpenGL rendering method. The second option is the :py:class:`~ovito.vis.TachyonRenderer`, which is
a software-only raytracing engine and which is able to produce better looking results in many cases.
Finally, the :py:class:`~ovito.vis.POVRayRenderer` offloads the rendering to the external `POV-Ray <http://www.povray.org/>`__
program, which must be installed on the local computer. 
Each of these rendering backends has specific parameters, and you can access the current renderer 
through the :py:attr:`RenderSettings.renderer <ovito.vis.RenderSettings.renderer>` attribute::

    settings.renderer = TachyonRenderer() # Activate the TachyonRenderer backend
    settings.renderer.shadows = False     # Turn off cast shadows
    
After the render settings have been specified, we can let OVITO render the image by calling 
:py:meth:`Viewport.render() <ovito.vis.Viewport.render>`::

    vp.render(settings)

Note that :py:meth:`~ovito.vis.Viewport.render` returns a `QImage <http://pyqt.sourceforge.net/Docs/PyQt5/api/qimage.html>`__,
giving you the possibility to manipulate the rendered picture before saving it to disk.
