# Load dependencies
import ovito.vis.stdobj
import ovito.vis.mesh
import ovito.vis.grid
import ovito.vis.stdmod
import ovito.vis.particles

# Load the native code modules.
from ovito.plugins.CrystalAnalysisPython import DislocationVis

# Inject selected classes into parent module.
ovito.vis.DislocationVis = DislocationVis
ovito.vis.__all__ += ['DislocationVis']

# Inject enum types.
DislocationVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
