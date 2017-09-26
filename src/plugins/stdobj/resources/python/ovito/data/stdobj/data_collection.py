# Load dependencies
import ovito
from ovito.data import DataCollection

# Load the native code module
from ovito.plugins.StdObj import SimulationCell

# Implementation of the DataCollection.cell attribute.
# Here for backward compatibility with OVITO 2.9.0.
def _DataCollection_cell(self):
    return self.find(SimulationCell)
DataCollection.cell = property(_DataCollection_cell)
