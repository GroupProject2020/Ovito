import ovito.data

# Load the native code module
from ovito.plugins.Particles import TrajectoryLineGenerator
from ovito.plugins.Particles.Modifiers import ReferenceConfigurationModifier

# Inject selected classes into parent module.
ovito.pipeline.TrajectoryLineGenerator = TrajectoryLineGenerator
ovito.pipeline.ReferenceConfigurationModifier = ReferenceConfigurationModifier

ovito.pipeline.__all__ += ['TrajectoryLineGenerator', 'ReferenceConfigurationModifier']

assert(issubclass(TrajectoryLineGenerator, ovito.data.DataCollection))
