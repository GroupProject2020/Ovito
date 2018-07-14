"""
This module contains classes related to :ref:`data visualization and rendering <rendering_intro>`.

**Rendering:**

  * :py:class:`Viewport`

**Rendering engines:**

  * :py:class:`OpenGLRenderer`
  * :py:class:`TachyonRenderer`
  * :py:class:`OSPRayRenderer`
  * :py:class:`POVRayRenderer`

**Data visualization elements:**

  * :py:class:`DataVis` (base class for all visual elements)
  * :py:class:`BondsVis`
  * :py:class:`DislocationVis`
  * :py:class:`ParticlesVis`
  * :py:class:`SimulationCellVis`
  * :py:class:`SurfaceMeshVis`
  * :py:class:`TrajectoryVis`
  * :py:class:`VectorVis`

**Viewport overlays:**

  * :py:class:`ColorLegendOverlay`
  * :py:class:`CoordinateTripodOverlay`
  * :py:class:`PythonViewportOverlay`
  * :py:class:`TextLabelOverlay`

"""

import sip
import PyQt5.QtGui

# Load the native modules.
from ..plugins.PyScript import (RenderSettings, Viewport, ViewportConfiguration, OpenGLRenderer, 
                                DataVis, CoordinateTripodOverlay, PythonViewportOverlay, TextLabelOverlay,
                                FrameBuffer)

import ovito

__all__ = ['RenderSettings', 'Viewport', 'ViewportConfiguration', 'OpenGLRenderer', 'DataVis',
        'CoordinateTripodOverlay', 'PythonViewportOverlay', 'TextLabelOverlay']

def _Viewport_render_image(self, size=(640,480), frame=0, filename=None, background=(1.0,1.0,1.0), alpha=False, renderer=None):
    """ Renders an image of the viewport's view.
    
        :param size: A pair of integers specifying the horizontal and vertical dimensions of the output image in pixels. 
        :param int frame: The animation frame to render. Numbering starts at 0. See the :py:attr:`FileSource.num_frames <ovito.pipeline.FileSource.num_frames>` property for the number of loaded animation frames.
        :param str filename: The filename under which the rendered image should be saved (optional).
                             Supported output formats are: :file:`.png`, :file:`.jpeg` and :file:`.tiff`.
        :param background: A triplet of RGB values in the range [0,1] specifying the background color of the rendered image. 
        :param alpha: If true, the background is made transparent so that the rendered image may later be superimposed on a different backdrop.
                      When using this option, make sure to save the image in the PNG format in order to preserve the transparency information.
        :param renderer: The rendering engine to use. If set to ``None``, either OpenGL or Tachyon are used, 
                         depending on the availablity of OpenGL in the current execution context.
        :returns: A `QImage <http://pyqt.sourceforge.net/Docs/PyQt5/api/qimage.html>`__ object containing the rendered picture; 
                  or ``None`` if the rendering operation was canceled by the user.

        **Populating the scene**

        Before rendering an image using this method, you should make sure the three-dimensional contains some
        visible objects. Typically this involves calling the :py:meth:`Pipeline.add_to_scene() <ovito.pipeline.Pipeline.add_to_scene>`
        method on a pipeline to insert its output data into the scene::

           pipeline = import_file('simulation.dump')
           pipeline.add_to_scene()

        **Selecting a rendering engine**

        OVITO supports several different rendering backends for producing pictures of the three-dimensional scene:
            
            * :py:class:`OpenGLRenderer`
            * :py:class:`TachyonRenderer`
            * :py:class:`OSPRayRenderer`
            * :py:class:`POVRayRenderer`
        
        Each of these backends exhibits specific parameters that control the image quality and other aspect of the image
        generation process. Typically, you would create an instance of one of these renderer classes, configure it and pass
        it to the :py:meth:`!render_image()` method:

        .. literalinclude:: ../example_snippets/viewport_select_renderer.py
           :lines: 5-

        Note that the :py:class:`OpenGLRenderer` backend may not be available when you are executing the script in a
        headless environment, e.g. on a remote HPC cluster without X display and OpenGL support. 
        
        **Post-processing images**

        If the ``filename`` parameter is omitted, the method does not save the rendered image to disk.
        This gives you the opportunity to paint additional graphics on top before saving the 
        `QImage <http://pyqt.sourceforge.net/Docs/PyQt5/api/qimage.html>`__ later using its ``save()`` method:
        
        .. literalinclude:: ../example_snippets/render_to_image.py

        As an alternative to the direct method demonstrated above, you can also make use of a :py:class:`PythonViewportOverlay`
        to paint custom graphics on top of rendered images. 
    """
    assert(len(size) == 2 and size[0]>0 and size[1]>0)
    assert(len(background) == 3)
    assert(background[0] >= 0.0 and background[0] <= 1.0)
    assert(background[1] >= 0.0 and background[1] <= 1.0)
    assert(background[2] >= 0.0 and background[2] <= 1.0)
    assert(renderer is None or isinstance(renderer, ovito.plugins.PyScript.SceneRenderer))

    # Rendering is a long-running operation, which is not permitted during viewport rendering or pipeline evaluation.
    # In these situations, the following function call will raise an exception.
    ovito.dataset.request_long_operation()

    # Configure a RenderSettings object:
    settings = RenderSettings()
    settings.output_image_width, settings.output_image_height = size
    settings.background_color = background
    if filename: 
        settings.output_filename = str(filename)
        settings.save_to_file = True
    if renderer:
        settings.renderer = renderer
    settings.range = RenderSettings.Range.CurrentFrame

    # Temporarily modify the global animation settings:
    old_frame = self.dataset.anim.current_frame
    self.dataset.anim.current_frame = int(frame)

    if len(self.dataset.pipelines) == 0:
        print("Warning: The scene to be rendered is empty. Did you forget to add a pipeline to the scene using Pipeline.add_to_scene()?")

    if ovito.gui_mode:
        # Use the frame buffer of the GUI window for rendering.
        fb_window = self.dataset.container.window.frame_buffer_window
        fb = fb_window.create_frame_buffer(settings.output_image_width, settings.output_image_height)
        fb_window.show_and_activate()
    else:
        # Create a temporary off-screen frame buffer.
        fb = FrameBuffer(settings.output_image_width, settings.output_image_height)
    
    try:
        if not self.dataset.render_scene(settings, self, fb, ovito.task_manager):
            return None
        else:
            return fb.image
    finally:
        # Restore animation settings to previous state:
        self.dataset.anim.current_frame = old_frame
Viewport.render_image = _Viewport_render_image

def _Viewport_render_anim(self, filename, size=(640,480), fps=10, background=(1.0,1.0,1.0), renderer=None, range=None, every_nth=1):
    """ Renders an animation sequence.
    
        :param str filename: The filename under which the rendered animation should be saved. 
                             Supported video formats are: :file:`.avi`, :file:`.mp4`, :file:`.mov` and :file:`.gif`.
                             Alternatively, an image format may be specified (:file:`.png`, :file:`.jpeg`).
                             In this case, a series of image files will be produced, one for each frame, which
                             may be combined into an animation using an external video encoding tool of your choice.
        :param size: The resolution of the movie in pixels. 
        :param fps: The number of frames per second of the encoded movie. This determines the playback speed of the animation. 
        :param background: An RGB triplet in the range [0,1] specifying the background color of the rendered movie. 
        :param renderer: The rendering engine to use. If none is specified, either OpenGL or Tachyon are used, 
                         depending on the availablity of OpenGL in the script execution context.  
        :param range: The interval of frames to render, specified in the form ``(from,to)``. 
                      Frame numbering starts at 0. If no interval is specified, the entire animation is rendered, i.e.
                      frame 0 through (:py:attr:`FileSource.num_frames <ovito.pipeline.FileSource.num_frames>`-1).
        :param every_nth: Frame skipping interval in case you don't want to render every frame of a very long animation. 
        :returns: ``True`` if successful; ``False`` if operation was canceled by the user.

        See also the :py:meth:`.render_image` method for a more detailed discussion of some of these parameters.
    """
    assert(len(size) == 2 and size[0]>0 and size[1]>0)
    assert(fps >= 1)
    assert(every_nth >= 1)
    assert(len(background) == 3)
    assert(background[0] >= 0.0 and background[0] <= 1.0)
    assert(background[1] >= 0.0 and background[1] <= 1.0)
    assert(background[2] >= 0.0 and background[2] <= 1.0)
    assert(renderer is None or isinstance(renderer, ovito.plugins.PyScript.SceneRenderer))
    
    # Rendering is a long-running operation, which is not permitted during viewport rendering or pipeline evaluation.
    # In these situations, the following function call will raise an exception.
    ovito.dataset.request_long_operation()
    
    # Configure a RenderSettings object:
    settings = RenderSettings()
    settings.output_image_width, settings.output_image_height = size
    settings.background_color = background
    settings.output_filename = str(filename)
    settings.save_to_file = True
    settings.frames_per_second = int(fps)
    if renderer:
        settings.renderer = renderer
    settings.every_nth_frame = int(every_nth)
    if range:
        settings.range = RenderSettings.Range.CustomInterval
        settings.custom_range_start, settings.custom_range_end = range
    else:
        settings.range = RenderSettings.Range.Animation

    if len(self.dataset.pipelines) == 0:
        print("Warning: The scene to be rendered is empty. Did you forget to add a pipeline to the scene using Pipeline.add_to_scene()?")

    if ovito.gui_mode:
        # Use the frame buffer of the GUI window for rendering.
        fb_window = self.dataset.container.window.frame_buffer_window
        fb = fb_window.create_frame_buffer(settings.output_image_width, settings.output_image_height)
        fb_window.show_and_activate()
    else:
        # Create a temporary off-screen frame buffer.
        fb = FrameBuffer(settings.output_image_width, settings.output_image_height)
    
    return self.dataset.render_scene(settings, self, fb, ovito.task_manager)
Viewport.render_anim = _Viewport_render_anim

# Here only for backward compatibility with OVITO 2.9.0:
def _get_RenderSettings_custom_range(self):
    """ 
    Specifies the range of animation frames to render if :py:attr:`.range` is set to ``CustomInterval``.
    
    :Default: ``(0,100)`` 
    """
    return (self.custom_range_start, self.custom_range_end)
def _set_RenderSettings_custom_range(self, range):
    if len(range) != 2: raise TypeError("Tuple or list of length two expected.")
    self.custom_range_start = range[0]
    self.custom_range_end = range[1]
RenderSettings.custom_range = property(_get_RenderSettings_custom_range, _set_RenderSettings_custom_range)

# Here only for backward compatibility with OVITO 2.9.0:
def _get_RenderSettings_size(self):
    """ 
    The size of the image or movie to be generated in pixels. 
    
    :Default: ``(640,480)`` 
    """
    return (self.output_image_width, self.output_image_height)
def _set_RenderSettings_size(self, size):
    if len(size) != 2: raise TypeError("Tuple or list of length two expected.")
    self.output_image_width = size[0]
    self.output_image_height = size[1]
RenderSettings.size = property(_get_RenderSettings_size, _set_RenderSettings_size)

# Here only for backward compatibility with OVITO 2.9.0:
def _get_RenderSettings_filename(self):
    """ 
    A string specifying the file path under which the rendered image or movie should be saved.
    
    :Default: ``None``
    """
    if self.save_to_file and self.output_filename: return self.output_filename
    else: return None
def _set_RenderSettings_filename(self, filename):
    if filename:
        self.output_filename = filename
        self.save_to_file = True
    else:
        self.save_to_file = False
RenderSettings.filename = property(_get_RenderSettings_filename, _set_RenderSettings_filename)

# Implement FrameBuffer.image property (requires conversion to SIP).
def _get_FrameBuffer_image(self):
    return PyQt5.QtGui.QImage(sip.wrapinstance(self._image, PyQt5.QtGui.QImage))
FrameBuffer.image = property(_get_FrameBuffer_image)

# Here only for backward compatibility with OVITO 2.9.0:
def _Viewport_render(self, settings = None):
    # Renders an image or movie of the viewport's view.
    #
    #    :param settings: A settings object, which specifies the resolution, background color, output filename etc. of the image to be rendered. 
    #                     If ``None``, the global settings are used (given by :py:attr:`DataSet.render_settings <ovito.DataSet.render_settings>`).
    #    :type settings: :py:class:`RenderSettings`
    #    :returns: A `QImage <http://pyqt.sourceforge.net/Docs/PyQt5/api/qimage.html>`__ object on success, which contains the rendered picture; 
    #              ``None`` if the rendering operation has been canceled by the user.
    if settings is None:
        settings = self.dataset.render_settings
    elif isinstance(settings, dict):
        settings = RenderSettings(settings)
    if len(self.dataset.pipelines) == 0:
        print("Warning: The scene to be rendered is empty. Did you forget to add a pipeline to the scene using Pipeline.add_to_scene()?")
    if ovito.gui_mode:
        # Use the frame buffer of the GUI window for rendering.
        fb_window = self.dataset.container.window.frame_buffer_window
        fb = fb_window.create_frame_buffer(settings.output_image_width, settings.output_image_height)
        fb_window.show_and_activate()
    else:
        # Create a temporary off-screen frame buffer.
        fb = FrameBuffer(settings.size[0], settings.size[1])
    if not self.dataset.render_scene(settings, self, fb, ovito.task_manager):
        return None
    return fb.image
Viewport.render = _Viewport_render

# Here only for backward compatibility with OVITO 2.9.0:
def _ViewportConfiguration__len__(self):
    return len(self.viewports)
ViewportConfiguration.__len__ = _ViewportConfiguration__len__

# Here only for backward compatibility with OVITO 2.9.0:
def _ViewportConfiguration__iter__(self):
    return self.viewports.__iter__()
ViewportConfiguration.__iter__ = _ViewportConfiguration__iter__

# Here only for backward compatibility with OVITO 2.9.0:
def _ViewportConfiguration__getitem__(self, index):
    return self.viewports[index]
ViewportConfiguration.__getitem__ = _ViewportConfiguration__getitem__
