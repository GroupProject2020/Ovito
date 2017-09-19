# Load dependencies
import ovito.modifiers

# Load the native code modules.
from ovito.plugins.Grid import CreateIsosurfaceModifier

# Inject modifier classes into parent module.
ovito.modifiers.CreateIsosurfaceModifier = CreateIsosurfaceModifier
ovito.modifiers.__all__ += ['CreateIsosurfaceModifier']
