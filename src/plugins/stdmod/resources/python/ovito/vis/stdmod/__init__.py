# Load dependencies
import ovito.vis.stdobj

# Load the native code module
from ovito.plugins.StdMod import ColorLegendOverlay

# Inject selected classes into parent module.
ovito.vis.ColorLegendOverlay = ColorLegendOverlay
ovito.vis.__all__ += ['ColorLegendOverlay']
