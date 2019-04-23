.. _rendering_intro:

===================================
Rendering & visualization
===================================

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

A visual element is attached to the :py:class:`~ovito.data.DataObject` that it should visualize, and you access it through the :py:attr:`~ovito.data.DataObject.vis`
attribute::

    >>> data.cell                                # This is the data object
    <SimulationCell at 0x7f9a414c8060>

    >>> data.cell.vis                            # This is the attached visual element
    <SimulationCellVis at 0x7fc3650a1c20>

    >>> data.cell.vis.rendering_color = (1,0,0)  # Giving the simulation box a red color

All display objects are derived from the :py:class:`~ovito.vis.DataVis` base class, which defines
the :py:attr:`~ovito.vis.DataVis.enabled` attribute turning the rendering of the data object on or off::

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
of a pipeline is generated. To render a picture of the scene, you typically create a new :py:class:`~ovito.vis.Viewport`
object and configure it by setting the camera position and orientation::

    import math
    from ovito.vis import Viewport

    vp = Viewport()
    vp.type = Viewport.Type.Perspective
    vp.camera_pos = (-100, -150, 150)
    vp.camera_dir = (2, 3, -3)
    vp.fov = math.radians(60.0)

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

Rendering of images and movies is done using the :py:meth:`Viewport.render_image() <ovito.vis.Viewport.render_image>` and
:py:meth:`Viewport.render_anim() <ovito.vis.Viewport.render_anim>` methods::

    vp.render_image(size=(800,600), filename="figure.png", background=(0,0,0), frame=8)
    vp.render_movie(size=(800,600), filename="animation.avi", fps=20)

OVITO provides several different :ovitoman:`rendering engines <../../rendering>`, which differ in terms of speed and visual quality.
The default rendering engine is the :py:class:`~ovito.vis.OpenGLRenderer`, which implements a fast, hardware-accelerated
OpenGL rendering method. See the :py:mod:`ovito.vis` module for the list of other available engines. To use one of them,
you have to create a renderer object and configure its specific parameters, and finally pass the renderer object to the
viewport rendering function::

    tachyon = TachyonRenderer(shadows=False, direct_light_intensity=1.1)
    vp.render_image(filename="figure.png", background=(1,1,1), renderer=tachyon)

