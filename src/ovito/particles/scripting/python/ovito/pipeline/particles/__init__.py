# Load dependencies
import ovito.pipeline
import ovito.pipeline.stdobj
import ovito.pipeline.stdmod
import ovito.pipeline.mesh
import ovito.pipeline.grid

# Load the native code module
from ovito.plugins.ParticlesPython import ReferenceConfigurationModifier

# Inject selected classes into parent module.
ovito.pipeline.ReferenceConfigurationModifier = ReferenceConfigurationModifier

ovito.pipeline.__all__ += ['ReferenceConfigurationModifier']
