# Load dependencies
import ovito.vis.mesh
import ovito.vis.particles

# Load the native code modules.
from ovito.plugins.CrystalAnalysisPython import DislocationVis, PartitionMeshVis

# Inject selected classes into parent module.
ovito.vis.DislocationVis = DislocationVis
ovito.vis.PartitionMeshVis = PartitionMeshVis
ovito.vis.__all__ += ['DislocationVis', 'PartitionMeshVis']

# Inject enum types.
DislocationVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
