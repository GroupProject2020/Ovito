"""
This module contains all modifiers available in OVITO. See :ref:`this introduction page <modifiers_overview>` to learn more
about modifiers and the data pipeline system of OVITO. 

The abstract base class of all modifier types is :py:class:`~ovito.pipeline.Modifier`.
Typically, you create a modifier instance, configure its parameters and insert it into a
data :py:class:`~ovito.pipeline.Pipeline` as follows::

    mod = AssignColorModifier()
    mod.color = (0.2, 1.0, 0.9)
    pipeline.modifiers.append(mod)
    
The following modifier types are available. Please consult the `OVITO user manual <../../particles.modifiers.html>`_ for a
more in-depth description of what these modifiers do.

============================================== =========================================
Python class name                              User interface name
============================================== =========================================
:py:class:`AffineTransformationModifier`       :guilabel:`Affine transformation`
:py:class:`AmbientOcclusionModifier`           :guilabel:`Ambient occlusion`
:py:class:`AssignColorModifier`                :guilabel:`Assign color`
:py:class:`AtomicStrainModifier`               :guilabel:`Atomic strain`
:py:class:`BinAndReduceModifier`               :guilabel:`Bin and reduce`
:py:class:`BondAngleAnalysisModifier`          :guilabel:`Bond-angle analysis`
:py:class:`CalculateDisplacementsModifier`     :guilabel:`Displacement vectors`
:py:class:`CentroSymmetryModifier`             :guilabel:`Centrosymmetry parameter`
:py:class:`ClearSelectionModifier`             :guilabel:`Clear selection`
:py:class:`ClusterAnalysisModifier`            :guilabel:`Cluster analysis`
:py:class:`ColorCodingModifier`                :guilabel:`Color coding`
:py:class:`CombineParticleSetsModifier`        :guilabel:`Combine particle sets`
:py:class:`CommonNeighborAnalysisModifier`     :guilabel:`Common neighbor analysis`
:py:class:`ComputeBondLengthsModifier`         :guilabel:`Compute bond lengths`
:py:class:`ComputePropertyModifier`            :guilabel:`Compute property`
:py:class:`ConstructSurfaceModifier`           :guilabel:`Construct surface mesh`
:py:class:`CoordinationNumberModifier`         :guilabel:`Coordination analysis`
:py:class:`CoordinationPolyhedraModifier`      :guilabel:`Coordination polyhedra`
:py:class:`CreateBondsModifier`                :guilabel:`Create bonds`
:py:class:`CreateIsosurfaceModifier`           :guilabel:`Create isosurface`
:py:class:`DeleteSelectedModifier`             :guilabel:`Delete selected` 
:py:class:`DislocationAnalysisModifier`        :guilabel:`Dislocation analysis (DXA)`
:py:class:`ElasticStrainModifier`              :guilabel:`Elastic strain calculation`
:py:class:`ExpandSelectionModifier`            :guilabel:`Expand selection`
:py:class:`ExpressionSelectionModifier`        :guilabel:`Expression selection`
:py:class:`FreezePropertyModifier`             :guilabel:`Freeze property`
:py:class:`HistogramModifier`                  :guilabel:`Histogram`
:py:class:`IdentifyDiamondModifier`            :guilabel:`Identify diamond structure`
:py:class:`InvertSelectionModifier`            :guilabel:`Invert selection`
:py:class:`LoadTrajectoryModifier`             :guilabel:`Load trajectory`
:py:class:`ManualSelectionModifier`            :guilabel:`Manual selection`
:py:class:`PolyhedralTemplateMatchingModifier` :guilabel:`Polyhedral template matching`
:py:class:`PythonScriptModifier`               :guilabel:`Python script`
:py:class:`ReplicateModifier`                  :guilabel:`Replicate`
:py:class:`ScatterPlotModifier`                :guilabel:`Scatter plot`
:py:class:`SelectTypeModifier`                 :guilabel:`Select type`
:py:class:`SliceModifier`                      :guilabel:`Slice`
:py:class:`VoronoiAnalysisModifier`            :guilabel:`Voronoi analysis`
:py:class:`VoroTopModifier`                    :guilabel:`VoroTop analysis`
:py:class:`WignerSeitzAnalysisModifier`        :guilabel:`Wigner-Seitz defect analysis`
:py:class:`WrapPeriodicImagesModifier`         :guilabel:`Wrap at periodic boundaries`
============================================== =========================================

*Note that some analysis modifiers of the graphical version of OVITO are missing in the list above and are not accessible from Python. 
That is because they perform computations that can be achieved equally well using the Numpy module
in a more straightforward manner.*

"""

# Load the native module.
from ..plugins.PyScript.Scene import (PythonScriptModifier, SliceModifier, AffineTransformationModifier,
                                    ClearSelectionModifier, InvertSelectionModifier, ColorCodingModifier,
                                    AssignColorModifier, DeleteSelectedModifier, ScatterPlotModifier,
                                    ReplicateModifier)

# Load submodules.
from .select_type_modifier import SelectTypeModifier
from .histogram_modifier import HistogramModifier

__all__ = ['PythonScriptModifier', 'SliceModifier', 'AffineTransformationModifier', 
            'ClearSelectionModifier', 'InvertSelectionModifier', 'ColorCodingModifier',
            'AssignColorModifier', 'DeleteSelectedModifier', 'SelectTypeModifier', 'HistogramModifier', 
            'ScatterPlotModifier', 'ReplicateModifier']

# For backward compatibility with OVITO 2.9.0:
def _ColorCodingModifier_set_particle_property(self, v): self.property = v
ColorCodingModifier.particle_property = property(lambda self: self.property, _ColorCodingModifier_set_particle_property)
def _ColorCodingModifier_set_bond_property(self, v): 
    self.operate_on = 'bonds'
    self.property = v
ColorCodingModifier.bond_property = property(lambda self: self.property, _ColorCodingModifier_set_bond_property)
class _ColorCodingModifier_AssignmentMode:
    Particles = 'particles'
    Bonds = 'bonds'
    Vectors = 'vectors'
ColorCodingModifier.AssignmentMode = _ColorCodingModifier_AssignmentMode
def _ColorCodingModifier_set_assign_to(self, v): self.operate_on = v
ColorCodingModifier.assign_to = property(lambda self: self.operate_on, _ColorCodingModifier_set_assign_to)
    
# For backward compatibility with OVITO 2.9.0:
def _AffineTransformationModifier_set_transform_particles(self, v): self.operate_on.add('particles')
AffineTransformationModifier.transform_particles = property(lambda self: 'particles' in self.operate_on, _AffineTransformationModifier_set_transform_particles)
def _AffineTransformationModifier_set_transform_box(self, v): self.operate_on.add('cell')
AffineTransformationModifier.transform_box = property(lambda self: 'box' in self.operate_on, _AffineTransformationModifier_set_transform_box)
def _AffineTransformationModifier_set_transform_surface(self, v): self.operate_on.add('surfaces')
AffineTransformationModifier.transform_surface = property(lambda self: 'surfaces' in self.operate_on, _AffineTransformationModifier_set_transform_surface)
def _AffineTransformationModifier_set_transform_vector_properties(self, v): self.operate_on.add('vector_properties')
AffineTransformationModifier.transform_vector_properties = property(lambda self: 'vector_properties' in self.operate_on, _AffineTransformationModifier_set_transform_vector_properties)

# For backward compatibility with OVITO 2.9.0:
DeleteSelectedParticlesModifier = DeleteSelectedModifier
SelectParticleTypeModifier = SelectTypeModifier
ShowPeriodicImagesModifier = ReplicateModifier
__all__ += ['DeleteSelectedParticlesModifier', 'SelectParticleTypeModifier', 'ShowPeriodicImagesModifier']