# Load dependencies
import ovito.modifiers
import ovito.modifiers.particles

# Load the native code modules.
import ovito.plugins.Particles
import ovito.plugins.CorrelationFunctionPlugin

# Inject modifier classes into parent module.
ovito.modifiers.CorrelationFunctionModifier = ovito.plugins.CorrelationFunctionPlugin.CorrelationFunctionModifier
ovito.modifiers.__all__ += ['CorrelationFunctionModifier']
