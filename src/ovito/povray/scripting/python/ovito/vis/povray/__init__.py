# Load dependencies
import ovito.vis

# Load the native code module
from ovito.plugins.POVRayPython import POVRayRenderer

# Inject POVRayRenderer class into parent module.
ovito.vis.POVRayRenderer = POVRayRenderer
ovito.vis.__all__ += ['POVRayRenderer']