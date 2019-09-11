# Load dependencies
import ovito.vis

# Load the native module.
from ovito.plugins.StdObjPython import SimulationCellVis

# Inject selected classes into parent module.
ovito.vis.SimulationCellVis = SimulationCellVis
ovito.vis.__all__ += ['SimulationCellVis']
