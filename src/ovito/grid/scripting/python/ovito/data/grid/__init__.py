# Load dependencies
import ovito.data.stdobj
import ovito.data.stdmod
import ovito.data.mesh

# Load the native code module
from ovito.plugins.GridPython import VoxelGrid

# Load submodules.
from .data_collection import DataCollection

# Inject selected classes into parent module.
ovito.data.VoxelGrid = VoxelGrid
ovito.data.__all__ += ['VoxelGrid']