# Load dependencies
import ovito.modifiers
import ovito.modifiers.particles

# Load the native code modules.
import ovito.plugins.Particles
import ovito.plugins.VoroTop

# Inject modifier classes into parent module.
ovito.modifiers.VoroTopModifier = ovito.plugins.VoroTop.VoroTopModifier
ovito.modifiers.__all__ += ['VoroTopModifier']
