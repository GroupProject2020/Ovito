# Load dependencies
import ovito.modifiers
import ovito.modifiers.stdmod

# Load the native code modules.
import ovito.plugins.Particles

# Load submodules.
from .coordination_number_modifier import CoordinationNumberModifier
from .compute_property_modifier import ComputePropertyModifier

# Inject classes into parent module.
ovito.modifiers.AmbientOcclusionModifier = ovito.plugins.Particles.AmbientOcclusionModifier
ovito.modifiers.WrapPeriodicImagesModifier = ovito.plugins.Particles.WrapPeriodicImagesModifier
ovito.modifiers.ExpandSelectionModifier = ovito.plugins.Particles.ExpandSelectionModifier
ovito.modifiers.BinAndReduceModifier = ovito.plugins.Particles.BinAndReduceModifier
ovito.modifiers.StructureIdentificationModifier = ovito.plugins.Particles.StructureIdentificationModifier
ovito.modifiers.CommonNeighborAnalysisModifier = ovito.plugins.Particles.CommonNeighborAnalysisModifier
ovito.modifiers.BondAngleAnalysisModifier = ovito.plugins.Particles.BondAngleAnalysisModifier
ovito.modifiers.CreateBondsModifier = ovito.plugins.Particles.CreateBondsModifier
ovito.modifiers.CentroSymmetryModifier = ovito.plugins.Particles.CentroSymmetryModifier
ovito.modifiers.ClusterAnalysisModifier = ovito.plugins.Particles.ClusterAnalysisModifier
ovito.modifiers.CoordinationNumberModifier = ovito.plugins.Particles.CoordinationNumberModifier
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
ovito.modifiers.__all__ += ['AmbientOcclusionModifier', 
            'WrapPeriodicImagesModifier',
            'ExpandSelectionModifier',
            'BinAndReduceModifier',
            'StructureIdentificationModifier', 'CommonNeighborAnalysisModifier', 'BondAngleAnalysisModifier',
            'CreateBondsModifier', 'CentroSymmetryModifier', 'ClusterAnalysisModifier', 'CoordinationNumberModifier',
            'CalculateDisplacementsModifier', 'AtomicStrainModifier',
            'WignerSeitzAnalysisModifier', 'VoronoiAnalysisModifier', 'IdentifyDiamondModifier', 'LoadTrajectoryModifier',
            'PolyhedralTemplateMatchingModifier',
            'CoordinationPolyhedraModifier', 'InterpolateTrajectoryModifier', 'GenerateTrajectoryLinesModifier']
