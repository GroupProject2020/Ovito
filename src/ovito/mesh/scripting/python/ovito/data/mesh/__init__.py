# Load dependencies
import ovito.data
import ovito.data.stdobj

# Load the native code module
import ovito.plugins.MeshPython

# Load submodules.
from .surface_mesh import SurfaceMesh
from .data_collection import DataCollection

# Inject selected classes into parent module.
ovito.data.SurfaceMesh = SurfaceMesh
ovito.data.__all__ += ['SurfaceMesh']
