# Load dependencies
import ovito.data.stdobj
import ovito.data.mesh

# Load the native code module
import ovito.plugins.Grid

# Load submodules.
from .data_collection import DataCollection

# Inject selected classes into parent module.
ovito.data.VoxelGrid = ovito.plugins.Grid.VoxelGrid
ovito.data.__all__ += ['VoxelGrid']