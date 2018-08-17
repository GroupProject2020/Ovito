try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

# Load dependencies
import ovito
from ovito.data import DataCollection
from ovito.plugins.PyScript import CloneHelper

# Load the native code module
from ovito.plugins.StdObj import DataSeries

# Helper class that is part of the implementation of the DataCollection.series attribute.
class DataSeriesView(collections.Mapping):
    """
    A dictionary-like view of all :py:class:`DataSeries` objects in a :py:class:`DataCollection`.
    An instance of this class is returned by :py:attr:`DataCollection.series`.

    The class implements the ``collections.abc.Mapping`` interface. That means it can be used
    like a standard read-only Python ``dict`` object to access a data series by identifier, e.g.:

    .. literalinclude:: ../example_snippets/data_series_view.py
        :lines: 9-11
    """

    def __init__(self, data_collection):
        self._data = data_collection

    def __len__(self):
        # Count the number of DataSeries objects in the collection.
        return sum(isinstance(obj, DataSeries) for obj in self._data.objects)

    def __getitem__(self, key):
        for obj in self._data.objects:
            if isinstance(obj, DataSeries): 
                if obj.id == key:
                    return obj
        raise KeyError("The DataCollection does not contain any data series with the ID '%s'." % key)
    
    def __iter__(self):
        for obj in self._data.objects:
            if isinstance(obj, DataSeries):
                yield obj.id
    
    def __repr__(self):
        return repr(dict(self))
