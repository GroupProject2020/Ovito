# Load dependencies
import ovito.vis

# Load the native code module
import ovito.plugins.PyScript
from ovito.plugins.Particles import ParticleDisplay, VectorDisplay, BondsDisplay, TrajectoryLineDisplay

# Inject selected classes into parent module.
ovito.vis.ParticleDisplay = ParticleDisplay
ovito.vis.VectorDisplay = VectorDisplay
ovito.vis.BondsDisplay = BondsDisplay
ovito.vis.TrajectoryLineDisplay = TrajectoryLineDisplay
ovito.vis.__all__ += ['ParticleDisplay', 'VectorDisplay', 'BondsDisplay', 'TrajectoryLineDisplay']

# Inject enum types.
ovito.vis.VectorDisplay.Shading = ovito.plugins.PyScript.ArrowShadingMode
ovito.vis.BondsDisplay.Shading = ovito.plugins.PyScript.ArrowShadingMode
ovito.vis.TrajectoryLineDisplay.Shading = ovito.plugins.PyScript.ArrowShadingMode
