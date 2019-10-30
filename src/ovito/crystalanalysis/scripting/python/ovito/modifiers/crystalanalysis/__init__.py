# Load dependencies
import ovito.modifiers.stdobj
import ovito.modifiers.mesh
import ovito.modifiers.grid
import ovito.modifiers.stdmod
import ovito.modifiers.particles

# Load the native code modules.
from ovito.plugins.CrystalAnalysisPython import DislocationAnalysisModifier, ElasticStrainModifier

# Inject modifier classes into parent module.
ovito.modifiers.DislocationAnalysisModifier = DislocationAnalysisModifier
ovito.modifiers.ElasticStrainModifier = ElasticStrainModifier
#ovito.modifiers.GrainSegmentationModifier = GrainSegmentationModifier
ovito.modifiers.__all__ += ['DislocationAnalysisModifier', 'ElasticStrainModifier']
#ovito.modifiers.__all__ += ['GrainSegmentationModifier']

# Copy enum list.
ElasticStrainModifier.Lattice = DislocationAnalysisModifier.Lattice
#GrainSegmentationModifier.Lattice = DislocationAnalysisModifier.Lattice
