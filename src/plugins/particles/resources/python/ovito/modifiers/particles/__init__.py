# Load dependencies
import ovito.modifiers
import ovito.modifiers.stdmod
import ovito.modifiers.stdobj
import ovito.modifiers.mesh
import ovito.modifiers.grid

# Load the native code modules.
import ovito.plugins.Particles

# Load submodules.
from .compute_property_modifier import ComputePropertyModifier

# Inject classes into parent module.
ovito.modifiers.AmbientOcclusionModifier = ovito.plugins.Particles.AmbientOcclusionModifier
ovito.modifiers.WrapPeriodicImagesModifier = ovito.plugins.Particles.WrapPeriodicImagesModifier
ovito.modifiers.ExpandSelectionModifier = ovito.plugins.Particles.ExpandSelectionModifier
ovito.modifiers.StructureIdentificationModifier = ovito.plugins.Particles.StructureIdentificationModifier
ovito.modifiers.CommonNeighborAnalysisModifier = ovito.plugins.Particles.CommonNeighborAnalysisModifier
ovito.modifiers.AcklandJonesModifier = ovito.plugins.Particles.AcklandJonesModifier
ovito.modifiers.CreateBondsModifier = ovito.plugins.Particles.CreateBondsModifier
ovito.modifiers.CentroSymmetryModifier = ovito.plugins.Particles.CentroSymmetryModifier
ovito.modifiers.ClusterAnalysisModifier = ovito.plugins.Particles.ClusterAnalysisModifier
ovito.modifiers.CoordinationAnalysisModifier = ovito.plugins.Particles.CoordinationAnalysisModifier
ovito.modifiers.CalculateDisplacementsModifier = ovito.plugins.Particles.CalculateDisplacementsModifier
ovito.modifiers.AtomicStrainModifier = ovito.plugins.Particles.AtomicStrainModifier
ovito.modifiers.WignerSeitzAnalysisModifier = ovito.plugins.Particles.WignerSeitzAnalysisModifier
ovito.modifiers.VoronoiAnalysisModifier = ovito.plugins.Particles.VoronoiAnalysisModifier
ovito.modifiers.IdentifyDiamondModifier = ovito.plugins.Particles.IdentifyDiamondModifier
ovito.modifiers.LoadTrajectoryModifier = ovito.plugins.Particles.LoadTrajectoryModifier
ovito.modifiers.PolyhedralTemplateMatchingModifier = ovito.plugins.Particles.PolyhedralTemplateMatchingModifier
ovito.modifiers.CoordinationPolyhedraModifier = ovito.plugins.Particles.CoordinationPolyhedraModifier
ovito.modifiers.InterpolateTrajectoryModifier = ovito.plugins.Particles.InterpolateTrajectoryModifier
ovito.modifiers.GenerateTrajectoryLinesModifier = ovito.plugins.Particles.GenerateTrajectoryLinesModifier
ovito.modifiers.__all__ += [
            'AmbientOcclusionModifier', 
            'WrapPeriodicImagesModifier',
            'ExpandSelectionModifier',
            'StructureIdentificationModifier', 
            'CommonNeighborAnalysisModifier', 
            'AcklandJonesModifier',
            'CreateBondsModifier', 
            'CentroSymmetryModifier', 
            'ClusterAnalysisModifier', 
            'CoordinationAnalysisModifier',
            'CalculateDisplacementsModifier', 
            'AtomicStrainModifier',
            'WignerSeitzAnalysisModifier', 
            'VoronoiAnalysisModifier', 
            'IdentifyDiamondModifier', 
            'LoadTrajectoryModifier',
            'PolyhedralTemplateMatchingModifier',
            'CoordinationPolyhedraModifier', 
            'InterpolateTrajectoryModifier', 
            'GenerateTrajectoryLinesModifier']

# For backward compatibility with OVITO 2.9.0:
ovito.modifiers.CoordinationNumberModifier = ovito.modifiers.CoordinationAnalysisModifier
ovito.modifiers.__all__ += ['CoordinationNumberModifier']
