# Load dependencies
import ovito.modifiers
import ovito.modifiers.particles

# Load the native code modules.
from ovito.plugins.CorrelationFunctionPluginPython import CorrelationFunctionModifier

# Inject modifier classes into parent module.
ovito.modifiers.CorrelationFunctionModifier = CorrelationFunctionModifier
ovito.modifiers.__all__ += ['CorrelationFunctionModifier']
