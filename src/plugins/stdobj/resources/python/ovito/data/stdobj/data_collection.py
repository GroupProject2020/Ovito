# Load dependencies
import ovito
from ovito.data import DataCollection
from ovito.data.data_objects_dict import DataObjectsDict

# Load the native code module
from ovito.plugins.StdObj import SimulationCell, DataSeries

# Implementation of the DataCollection.series attribute.
def _DataCollection_series(self):
    """
    Returns a :py:class:`DataObjectsDict` providing key-based access to all :py:class:`DataSeries` objects stored 
    in this :py:class:`!DataCollection`.
    """
    return DataObjectsDict(self, DataSeries)
DataCollection.series = property(_DataCollection_series)

# Implementation of the DataCollection.cell attribute.
def _DataCollection_cell(self):
    """ 
    Returns the :py:class:`SimulationCell` stored in this :py:class:`!DataCollection` or ``None`` if there is
    no simulation cell object. 
    """
    return self.find(SimulationCell)
DataCollection.cell = property(_DataCollection_cell)

# Implementation of the DataCollection.cell_ attribute.
DataCollection.cell_ = property(lambda self: self.make_mutable(self.cell))
