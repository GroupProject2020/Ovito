# Load dependencies
import ovito.modifiers
import ovito.modifiers.particles

# Load the native code modules.
from ovito.plugins.CorrelationFunctionPluginPython import SpatialCorrelationFunctionModifier

# Inject modifier classes into parent module.
ovito.modifiers.SpatialCorrelationFunctionModifier = SpatialCorrelationFunctionModifier
ovito.modifiers.__all__ += ['SpatialCorrelationFunctionModifier']

# For backward compatibility with OVITO 2.9.0:
ovito.modifiers.CorrelationFunctionModifier = SpatialCorrelationFunctionModifier
