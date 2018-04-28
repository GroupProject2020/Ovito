""" 
This root module contains the :py:class:`Scene` class, which serves as a "universe" for all actions
performed by a script. The scene object is accessible through the module-level :py:data:`ovito.scene` variable.
"""

import os.path
import sys
import pkgutil
import importlib

# Allow OVITO plugins and C++ extension modules to be spread over several installation directories.
_package_source_path = __path__ # Make a copy of the original path, which will be used below to automatically import all submodules.
__path__ = pkgutil.extend_path(__path__, __name__)

# Load the native module with the core bindings.
from .plugins.PyScript import (version, version_string, gui_mode, headless_mode, Scene, scene, dataset, task_manager)
from .plugins.PyScript import (Pipeline, RootSceneNode, PipelineObject, PipelineStatus)

# Load sub-modules (in the right order because there are dependencies between them)
import ovito.anim
import ovito.data
import ovito.vis
import ovito.modifiers
import ovito.pipeline
import ovito.io

# Load the bindings for the GUI classes when running in gui mode.
if ovito.gui_mode:
    import ovito.plugins.PyScriptGui

__all__ = ['version', 'version_string', 'scene', 'Scene']

# Load the whole OVITO package. This is required to make all Python bindings available.
for _, _name, _ in pkgutil.walk_packages(_package_source_path, __name__ + '.'):
    if _name.startswith("ovito.plugins"): continue  # Do not load C++ plugin modules at this point, only Python modules
    try:
        importlib.import_module(_name)
    except:
        print("Error while loading OVITO submodule %s:" % _name, sys.exc_info()[0])
        raise

def _Scene_pipelines(self):
    """ The list of :py:class:`~ovito.pipeline.Pipeline` objects that are currently part of the three-dimensional scene.
        Only pipelines in this list will display their output data in the viewports and in rendered images. You can add or remove a pipeline either by calling
        its :py:meth:`~ovito.pipeline.Pipeline.add_to_scene` and :py:meth:`~ovito.pipeline.Pipeline.remove_from_scene` methods or by directly manipulating this list using 
        standard Python ``append()`` and ``del`` statements:

        .. literalinclude:: ../example_snippets/scene_pipelines.py
    """
    return self.scene_root.children
Scene.pipelines = property(_Scene_pipelines)

def _get_Scene_selected_pipeline(self):
    """ The :py:class:`~ovito.pipeline.Pipeline` that is currently selected in the graphical OVITO program, 
        or ``None`` if no pipeline is selected. Typically, this is the last pipeline that was added to the scene using
        :py:meth:`Pipeline.add_to_scene() <ovito.pipeline.Pipeline.add_to_scene>`.
    """
    return self.selection.nodes[0] if self.selection.nodes else None
def _set_Scene_selected_pipeline(self, pipeline):
    """ Sets the :py:class:`~ovito.pipeline.Pipeline` that is currently selected in the graphical user interface of OVITO. """
    if pipeline: self.selection.nodes = [pipeline]
    else: del self.selection.nodes[:]
Scene.selected_pipeline = property(_get_Scene_selected_pipeline, _set_Scene_selected_pipeline)

# For backward compatibility with OVITO 2.9.0:
Scene.scene_nodes = property(lambda self: self.pipelines)
Scene.selected_node = property(lambda self: self.selected_pipeline, _set_Scene_selected_pipeline)
ObjectNode = ovito.pipeline.Pipeline
DataSet = ovito.Scene
