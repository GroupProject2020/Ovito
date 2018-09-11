# Load dependencies
import ovito
from ovito.data import DataCollection
from .data_series_view import DataSeriesView

# Load the native code module
from ovito.plugins.StdObj import SimulationCell

# Implementation of the DataCollection.series attribute.
def _DataCollection_series(self):
    """
    Returns a :py:class:`DataSeriesView` providing name-based access to all :py:class:`DataSeries` objects stored 
    in this :py:class:`!DataCollection`.
    """
    return DataSeriesView(self)
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
