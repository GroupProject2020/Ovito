# Load dependencies
import ovito.modifiers.stdobj
import ovito.modifiers.mesh

# Load the native code modules.
from ovito.plugins.Grid import CreateIsosurfaceModifier, SpatialBinningModifier

# Inject modifier classes into parent module.
ovito.modifiers.CreateIsosurfaceModifier = CreateIsosurfaceModifier
ovito.modifiers.SpatialBinningModifier = SpatialBinningModifier
ovito.modifiers.__all__ += ['CreateIsosurfaceModifier', 'SpatialBinningModifier']

# For backward compatibility with OVITO 2.9.0:
ovito.modifiers.BinAndReduceModifier = SpatialBinningModifier
ovito.modifiers.__all__ += ['BinAndReduceModifier']
