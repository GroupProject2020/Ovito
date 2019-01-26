# Load dependencies
import ovito.modifiers.particles

# Load the native code module.
import ovito.plugins.VoroTopPython

# Inject modifier classes into parent module.
ovito.modifiers.VoroTopModifier = ovito.plugins.VoroTopPython.VoroTopModifier
ovito.modifiers.__all__ += ['VoroTopModifier']
