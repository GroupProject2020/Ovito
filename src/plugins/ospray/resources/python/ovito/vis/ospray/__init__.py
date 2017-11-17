# Load dependencies
import ovito.vis

# Load the native code module
import ovito.plugins.OSPRayRenderer

# Inject OSPRayRenderer class into parent module.
ovito.vis.OSPRayRenderer = ovito.plugins.OSPRayRenderer.OSPRayRenderer
ovito.vis.__all__ += ['OSPRayRenderer']
