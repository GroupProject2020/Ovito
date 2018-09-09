try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

# Load dependencies
import ovito

# Load the native code module
from ovito.plugins.StdObj import PropertyContainer

# Give the PropertyContainer class a dict-like interface.
PropertyContainer.__len__ = lambda self: len(self.properties)

def _PropertyContainer__iter__(self):
    for p in self.properties:
        yield p.name
PropertyContainer.__iter__ = _PropertyContainer__iter__

def _PropertyContainer__getitem__(self, key):
    for p in self.properties:
        if p.name == key:
            return p
    raise KeyError("Property container does not contain a property named '%s'." % key)
PropertyContainer.__getitem__ = _PropertyContainer__getitem__

def _PropertyContainer_get(self, key, default=None):
    try: return self[key]
    except KeyError: return default
PropertyContainer.get = _PropertyContainer_get

def _PropertyContainer_keys(self):
    return collections.KeysView(self)
PropertyContainer.keys = _PropertyContainer_keys

def _PropertyContainer_items(self):
    return collections.ItemsView(self)
PropertyContainer.items = _PropertyContainer_items

def _PropertyContainer_values(self):
    return collections.ValuesView(self)
PropertyContainer.values = _PropertyContainer_values

collections.Mapping.register(PropertyContainer)