# Load dependencies
import ovito.data

# Load the module's classes
from .simulation_cell import SimulationCell
from .property import Property
from .plot_data import PlotData
from .data_collection import DataCollection

# Inject selected classes into parent module.
ovito.data.SimulationCell = SimulationCell
ovito.data.Property = Property
ovito.data.PlotData = PlotData
ovito.data.__all__ += ['SimulationCell', 'Property', 'PlotData']
