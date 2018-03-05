""" 
This root module contains the :py:class:`DataSet` class, which serves as a "universe" for all
performed operations. At all times there is exactly one :py:class:`DataSet` instance that 
provides a context for the operations the Python script performs. It is accessible through the 
module-level :py:data:`ovito.dataset` attribute.

"""

import os.path
import sys
import pkgutil
import importlib

# Allow OVITO plugins and C++ extension modules to be spread over several installation directories.
_package_source_path = __path__ # Make a copy of the original path, which will be used below to automatically import all submodules.
__path__ = pkgutil.extend_path(__path__, __name__)

# Load the native module with the core bindings.
from .plugins.PyScript import (version, version_string, gui_mode, headless_mode, DataSet, dataset, task_manager)
from .plugins.PyScript import (Pipeline, RootSceneNode, PipelineObject, PipelineStatus)

# Load sub-modules (in the right order because there are dependencies among them)
import ovito.anim
import ovito.data
import ovito.vis
import ovito.modifiers
import ovito.pipeline
import ovito.io

# Load the bindings for the GUI classes when running in gui mode.
if ovito.gui_mode:
    import ovito.plugins.PyScriptGui

__all__ = ['version', 'version_string', 'dataset', 'DataSet']

# Load the whole OVITO package. This is required to make all Python bindings available.
for _, _name, _ in pkgutil.walk_packages(_package_source_path, __name__ + '.'):
    if _name.startswith("ovito.linalg"): continue  # For backward compatibility with OVITO 2.7.1. The old 'ovito.linalg' module has been deprecated but may still be present in some existing OVITO installations.
    if _name.startswith("ovito.plugins"): continue  # Do not load C++ plugin modules at this point, only Python modules
    try:
        importlib.import_module(_name)
    except:
        print("Error while loading OVITO submodule %s:" % _name, sys.exc_info()[0])
        raise

def _DataSet_scene_pipelines(self):
    """ The list of :py:class:`~ovito.pipeline.Pipeline` objects that are currently part of the three-dimensional scene.
        Only data of pipelines in this list is visible in the viewports and in rendered images. You can add or remove pipelines either by calling
        their :py:meth:`~ovito.pipeline.Pipeline.add_to_scene` and :py:meth:`~ovito.pipeline.Pipeline.remove_from_scene` methods or by manipulating the :py:attr:`!scene_pipelines` list using 
        standard Python ``append()`` and ``del`` statements. 
    """
    return self.scene_root.children
DataSet.scene_pipelines = property(_DataSet_scene_pipelines)

def _get_DataSet_selected_pipeline(self):
    """ The :py:class:`~ovito.pipeline.Pipeline` that is currently selected in OVITO's graphical user interface, 
        or ``None`` if no pipeline is selected. """
    return self.selection.nodes[0] if self.selection.nodes else None
def _set_DataSet_selected_pipeline(self, node):
    """ Sets the :py:class:`~ovito.pipeline.Pipeline` that is currently selected in the graphical user interface of OVITO. """
    if node: self.selection.nodes = [node]
    else: del self.selection.nodes[:]
DataSet.selected_pipeline = property(_get_DataSet_selected_pipeline, _set_DataSet_selected_pipeline)

# For backward compatibility with OVITO 2.9.0:
DataSet.scene_nodes = property(lambda self: self.scene_pipelines)
DataSet.selected_node = property(lambda self: self.selected_pipeline, _set_DataSet_selected_pipeline)
ObjectNode = ovito.pipeline.Pipeline
