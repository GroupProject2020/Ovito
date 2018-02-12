# Load dependencies
import ovito.vis

# Load the native code module
import ovito.plugins.PyScript
from ovito.plugins.Particles import ParticleVis, VectorVis, BondsVis, TrajectoryLineVis

# Inject selected classes into parent module.
ovito.vis.ParticleVis = ParticleVis
ovito.vis.VectorVis = VectorVis
ovito.vis.BondsVis = BondsVis
ovito.vis.TrajectoryLineVis = TrajectoryLineVis
ovito.vis.__all__ += ['ParticleVis', 'VectorVis', 'BondsVis', 'TrajectoryLineVis']

# Inject enum types.
ovito.vis.VectorVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
ovito.vis.BondsVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
ovito.vis.TrajectoryLineVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
