# Load dependencies
import ovito.vis
import ovito.vis.mesh

# Load the native code modules.
import ovito.plugins.PyScript
import ovito.plugins.Particles
import ovito.plugins.CrystalAnalysis

# Inject selected classes into parent module.
ovito.vis.DislocationDisplay = ovito.plugins.CrystalAnalysis.DislocationDisplay
ovito.vis.PartitionMeshDisplay = ovito.plugins.CrystalAnalysis.PartitionMeshDisplay
ovito.vis.__all__ += ['DislocationDisplay', 'PartitionMeshDisplay']

# Inject enum types.
ovito.vis.DislocationDisplay.Shading = ovito.plugins.PyScript.ArrowShadingMode
