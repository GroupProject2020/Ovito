# Load dependencies
import ovito.vis

# Load the native code module
import ovito.plugins.TachyonPython

# Inject TachyonRenderer class into parent module.
ovito.vis.TachyonRenderer = ovito.plugins.TachyonPython.TachyonRenderer
ovito.vis.__all__ += ['TachyonRenderer']
