# Load dependencies
import ovito.modifiers

# Load the native code modules.
import ovito.plugins.Particles
from ovito.plugins.Particles.Modifiers import *

# Load submodules.
from .coordination_number_modifier import CoordinationNumberModifier

# Inject classes into parent module.
ovito.modifiers.AmbientOcclusionModifier = AmbientOcclusionModifier
ovito.modifiers.WrapPeriodicImagesModifier = WrapPeriodicImagesModifier
ovito.modifiers.ComputePropertyModifier = ComputePropertyModifier
ovito.modifiers.FreezePropertyModifier = FreezePropertyModifier
ovito.modifiers.ManualSelectionModifier = ManualSelectionModifier
ovito.modifiers.ExpandSelectionModifier = ExpandSelectionModifier
ovito.modifiers.ExpressionSelectionModifier = ExpressionSelectionModifier
ovito.modifiers.BinAndReduceModifier = BinAndReduceModifier
ovito.modifiers.StructureIdentificationModifier = StructureIdentificationModifier
ovito.modifiers.CommonNeighborAnalysisModifier = CommonNeighborAnalysisModifier
ovito.modifiers.BondAngleAnalysisModifier = BondAngleAnalysisModifier
ovito.modifiers.CreateBondsModifier = CreateBondsModifier
ovito.modifiers.CentroSymmetryModifier = CentroSymmetryModifier
ovito.modifiers.ClusterAnalysisModifier = ClusterAnalysisModifier
ovito.modifiers.CoordinationNumberModifier = CoordinationNumberModifier
ovito.modifiers.CalculateDisplacementsModifier = CalculateDisplacementsModifier
ovito.modifiers.AtomicStrainModifier = AtomicStrainModifier
ovito.modifiers.WignerSeitzAnalysisModifier = WignerSeitzAnalysisModifier
ovito.modifiers.VoronoiAnalysisModifier = VoronoiAnalysisModifier
ovito.modifiers.IdentifyDiamondModifier = IdentifyDiamondModifier
ovito.modifiers.LoadTrajectoryModifier = LoadTrajectoryModifier
ovito.modifiers.CombineParticleSetsModifier = CombineParticleSetsModifier
ovito.modifiers.ComputeBondLengthsModifier = ComputeBondLengthsModifier
ovito.modifiers.PolyhedralTemplateMatchingModifier = PolyhedralTemplateMatchingModifier
ovito.modifiers.CoordinationPolyhedraModifier = CoordinationPolyhedraModifier
ovito.modifiers.__all__ += ['AmbientOcclusionModifier', 
            'WrapPeriodicImagesModifier', 'ComputePropertyModifier', 'FreezePropertyModifier',
            'ManualSelectionModifier', 'ExpandSelectionModifier',
            'ExpressionSelectionModifier', 'BinAndReduceModifier',
            'StructureIdentificationModifier', 'CommonNeighborAnalysisModifier', 'BondAngleAnalysisModifier',
            'CreateBondsModifier', 'CentroSymmetryModifier', 'ClusterAnalysisModifier', 'CoordinationNumberModifier',
            'CalculateDisplacementsModifier', 'AtomicStrainModifier',
            'WignerSeitzAnalysisModifier', 'VoronoiAnalysisModifier', 'IdentifyDiamondModifier', 'LoadTrajectoryModifier',
            'CombineParticleSetsModifier', 'ComputeBondLengthsModifier', 'PolyhedralTemplateMatchingModifier',
            'CoordinationPolyhedraModifier']
            
# For backward compatibility with OVITO 2.9.0:
ovito.modifiers.SelectExpressionModifier = ExpressionSelectionModifier
ovito.modifiers.__all__ += ['SelectExpressionModifier']
