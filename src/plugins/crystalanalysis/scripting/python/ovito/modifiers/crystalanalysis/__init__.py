# Load dependencies
import ovito.modifiers.particles

# Load the native code modules.
from ovito.plugins.CrystalAnalysisPython import ConstructSurfaceModifier, DislocationAnalysisModifier, ElasticStrainModifier

# Inject modifier classes into parent module.
ovito.modifiers.ConstructSurfaceModifier = ConstructSurfaceModifier
ovito.modifiers.DislocationAnalysisModifier = DislocationAnalysisModifier
ovito.modifiers.ElasticStrainModifier = ElasticStrainModifier
#ovito.modifiers.GrainSegmentationModifier = GrainSegmentationModifier
ovito.modifiers.__all__ += ['ConstructSurfaceModifier', 'DislocationAnalysisModifier',
            'ElasticStrainModifier']
#ovito.modifiers.__all__ += ['GrainSegmentationModifier']

# Copy enum list.
ElasticStrainModifier.Lattice = DislocationAnalysisModifier.Lattice
#GrainSegmentationModifier.Lattice = DislocationAnalysisModifier.Lattice
