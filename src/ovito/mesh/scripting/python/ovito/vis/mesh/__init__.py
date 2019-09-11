# Load dependencies
import ovito.vis
import ovito.vis.stdobj
import ovito.vis.stdmod

# Load the native code module
from ovito.plugins.MeshPython import SurfaceMeshVis

# Inject selected classes into parent module.
ovito.vis.SurfaceMeshVis = SurfaceMeshVis
ovito.vis.__all__ += ['SurfaceMeshVis']
