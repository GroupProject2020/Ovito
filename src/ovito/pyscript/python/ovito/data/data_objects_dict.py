try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

# Load dependencies
import ovito
from ovito.data import DataCollection

# Helper class that exposes all data objects of a given class stored in a DataCollection.
class DataObjectsDict(collections.Mapping):
    """
    A dictionary-like view of all :py:class:`DataObject` instances of a particular type in a :py:class:`DataCollection`.

    The class implements the ``collections.abc.Mapping`` interface. That means it can be used
    like a standard read-only Python ``dict`` object to access a data objects by identifier, e.g.:
    """

    def __init__(self, data_collection, data_object_class):
        self._data = data_collection
        self._data_object_class = data_object_class

    def __len__(self):
        # Count the number of data objects of the right type in the collection.
        return sum(isinstance(obj, self._data_object_class) for obj in self._data.objects)

    def __getitem__(self, key):
        # Returns the data object with the given identifier key.
        if key.endswith('_'):
            if not self._data.is_safe_to_modify:
                raise ValueError("Requesting a mutable version of a {} object is not possible for a data collection that itself is not mutable.".format(self._data_object_class.__name__))
            request_mutable = True
            key = key[:-1]
        else:
            request_mutable = False
        for obj in self._data.objects:
            if isinstance(obj, self._data_object_class):
                if obj.identifier == key:
                    if request_mutable:
                        return self._data.make_mutable(obj)
                    else:
                        return obj
        raise KeyError("No data object of type <{}> for key '{}'".format(self._data_object_class.__name__, str(key)))

    def __iter__(self):
        for obj in self._data.objects:
            if isinstance(obj, self._data_object_class):
                yield obj.identifier

    def __repr__(self):
        return repr(dict(self))
