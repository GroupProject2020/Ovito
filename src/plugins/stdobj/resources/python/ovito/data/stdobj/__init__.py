# Load dependencies
import ovito.data

# Load the module's classes
from .simulation_cell import SimulationCell
from .property import Property
from .data_series import DataSeries
from .data_collection import DataCollection
from .data_series_view import DataSeriesView

# Inject selected classes into parent module.
ovito.data.SimulationCell = SimulationCell
ovito.data.Property = Property
ovito.data.DataSeries = DataSeries
ovito.data.DataSeriesView = DataSeriesView
ovito.data.__all__ += ['SimulationCell', 'Property', 'DataSeries', 'DataSeriesView']
