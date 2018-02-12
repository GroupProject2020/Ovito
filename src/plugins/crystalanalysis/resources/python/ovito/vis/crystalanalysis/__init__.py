# Load dependencies
import ovito.vis
import ovito.vis.mesh

# Load the native code modules.
import ovito.plugins.PyScript
import ovito.plugins.Particles
import ovito.plugins.CrystalAnalysis

# Inject selected classes into parent module.
ovito.vis.DislocationVis = ovito.plugins.CrystalAnalysis.DislocationVis
ovito.vis.PartitionMeshVis = ovito.plugins.CrystalAnalysis.PartitionMeshVis
ovito.vis.__all__ += ['DislocationVis', 'PartitionMeshVis']

# Inject enum types.
ovito.vis.DislocationVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
