# Load dependencies
import ovito.vis

# Load the native code module
from ovito.plugins.OSPRayRendererPython import OSPRayRenderer

# Inject OSPRayRenderer class into parent module.
ovito.vis.OSPRayRenderer = OSPRayRenderer
ovito.vis.__all__ += ['OSPRayRenderer']
