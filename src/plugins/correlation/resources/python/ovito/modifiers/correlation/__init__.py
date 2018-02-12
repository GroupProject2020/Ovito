# Load dependencies
import ovito.modifiers
import ovito.modifiers.particles

# Load the native code modules.
import ovito.plugins.Particles
import ovito.plugins.CorrelationFunctionPlugin

# Load submodules.
from .correlation_function_modifier import CorrelationFunctionModifier

# Inject modifier classes into parent module.
ovito.modifiers.CorrelationFunctionModifier = CorrelationFunctionModifier
ovito.modifiers.__all__ += ['CorrelationFunctionModifier']
