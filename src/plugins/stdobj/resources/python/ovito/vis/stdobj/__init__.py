# Load dependencies
import ovito.vis

# Load the module's classes
from ovito.plugins.StdObj import SimulationCellDisplay

# Inject selected classes into parent module.
ovito.vis.SimulationCellDisplay = SimulationCellDisplay
ovito.vis.__all__ += ['SimulationCellDisplay']
