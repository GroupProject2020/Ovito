# Load dependencies
import ovito.vis

# Load the native code module
import ovito.plugins.PyScript
from ovito.plugins.Particles import ParticlesVis, VectorVis, BondsVis, TrajectoryVis

# Inject selected classes into parent module.
ovito.vis.ParticlesVis = ParticlesVis
ovito.vis.VectorVis = VectorVis
ovito.vis.BondsVis = BondsVis
ovito.vis.TrajectoryVis = TrajectoryVis
ovito.vis.__all__ += ['ParticlesVis', 'VectorVis', 'BondsVis', 'TrajectoryVis']

# Inject enum types.
ovito.vis.VectorVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
ovito.vis.BondsVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
ovito.vis.TrajectoryVis.Shading = ovito.plugins.PyScript.ArrowShadingMode
