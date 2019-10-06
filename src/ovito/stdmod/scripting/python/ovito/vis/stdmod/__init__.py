# Load dependencies
import ovito.vis
import ovito.vis.stdobj
import ovito.vis.mesh

# Load the native code module
from ovito.plugins.StdModPython import ColorLegendOverlay

# Inject selected classes into parent module.
ovito.vis.ColorLegendOverlay = ColorLegendOverlay
ovito.vis.__all__ += ['ColorLegendOverlay']
