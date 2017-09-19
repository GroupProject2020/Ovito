# Load dependencies
import ovito.vis

# Load the native code module
from ovito.plugins.Mesh import SurfaceMeshDisplay

# Inject selected classes into parent module.
ovito.vis.SurfaceMeshDisplay = SurfaceMeshDisplay
ovito.vis.__all__ += ['SurfaceMeshDisplay']
