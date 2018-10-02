# Load dependencies
import ovito.data

# Load the native module.
import ovito.plugins.StdObj

from .simulation_cell import SimulationCell
from .property import Property
from .property_container import PropertyContainer
from .data_series import DataSeries
from .data_collection import DataCollection

# Inject selected classes into parent module.
ovito.data.SimulationCell = SimulationCell
ovito.data.Property = Property
ovito.data.PropertyContainer = PropertyContainer
ovito.data.DataSeries = DataSeries
ovito.data.ElementType = ovito.plugins.StdObj.ElementType
ovito.data.__all__ += ['SimulationCell', 'Property', 'PropertyContainer', 'DataSeries', 'ElementType']
