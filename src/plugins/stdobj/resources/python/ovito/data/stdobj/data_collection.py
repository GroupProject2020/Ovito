# Load dependencies
import ovito
from ovito.data import DataCollection
from ovito.data.data_objects_dict import DataObjectsDict

# Load the native code module
from ovito.plugins.StdObj import SimulationCell, DataSeries

# Implementation of the DataCollection.series attribute.
def _DataCollection_series(self):
    """
    Returns a dictionary view providing key-based access to all :py:class:`DataSeries` objects of 
    this data collection. Each :py:class:`DataSeries` has a unique :py:attr:`~ovito.data.DataObject.identifier` key, 
    which can be used to look it up in the dictionary. 
    See the documentation of the modifier producing a data series to find out what is the right key, or use

    .. literalinclude:: ../example_snippets/data_collection_series.py
        :lines: 9-9               

    to see which identifier keys exist. Then retrieve the desired :py:class:`DataSeries` from the collection using its identifier key, e.g.

    .. literalinclude:: ../example_snippets/data_collection_series.py
        :lines: 14-15

    """
    return DataObjectsDict(self, DataSeries)
DataCollection.series = property(_DataCollection_series)

# Implementation of the DataCollection.cell attribute.
def _DataCollection_cell(self):
    """ 
    Returns the :py:class:`SimulationCell` object, which stores the cell vectors and periodic boundary
    condition flags, or ``None`` if there is no cell information associated with this data collection. 

    Note: The :py:class:`SimulationCell` data object may be read-only if it is currently shared by
    several data collections. Use the :py:attr:`!cell_` field instead to request a mutable cell object 
    if you intend to modify it.
    """
    return self.find(SimulationCell)
DataCollection.cell = property(_DataCollection_cell)

# Implementation of the DataCollection.cell_ attribute.
DataCollection.cell_ = property(lambda self: self.make_mutable(self.cell))
