# Load dependencies
import ovito.vis
import ovito.vis.stdobj

# Load the native code module
from ovito.plugins.MeshPython import SurfaceMeshVis

# Inject selected classes into parent module.
ovito.vis.SurfaceMeshVis = SurfaceMeshVis
ovito.vis.__all__ += ['SurfaceMeshVis']
