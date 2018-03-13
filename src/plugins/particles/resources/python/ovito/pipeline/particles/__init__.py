import ovito.data

# Load the native code module
from ovito.plugins.Particles import ReferenceConfigurationModifier

# Inject selected classes into parent module.
ovito.pipeline.ReferenceConfigurationModifier = ReferenceConfigurationModifier

ovito.pipeline.__all__ += ['ReferenceConfigurationModifier']
