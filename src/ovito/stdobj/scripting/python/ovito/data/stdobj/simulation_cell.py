# Load dependencies.
import ovito
import numpy

# Load the native code module.
from ovito.plugins.StdObjPython import SimulationCell

from .property import PropertyView

# Implement indexing for simulation cell objects.
def _SimulationCell__getitem__(self, idx):
    return self.__array__()[idx]
SimulationCell.__getitem__ = _SimulationCell__getitem__

# Indexed assignment.
def _SimulationCell__setitem__(self, idx, value):
    # Write access to a cell matrix requires a 'with' Python statement.
    with self: self.__array__impl__()[idx] = value
SimulationCell.__setitem__ = _SimulationCell__setitem__

# Implementation of the Numpy array protocol:
def _SimulationCell__array__(self, dtype=None):
    return PropertyView(self.__array__impl__(dtype), self)
SimulationCell.__array__ = _SimulationCell__array__

# Implement iteration for simulation cell matrix.
SimulationCell.__iter__ = lambda self: iter(self.__array__())

# Standard array operators.
SimulationCell.__eq__ = lambda self, y: self.__array__impl__().__eq__(y)
SimulationCell.__ne__ = lambda self, y: self.__array__impl__().__ne__(y)
SimulationCell.__lt__ = lambda self, y: self.__array__impl__().__lt__(y)
SimulationCell.__le__ = lambda self, y: self.__array__impl__().__le__(y)
SimulationCell.__gt__ = lambda self, y: self.__array__impl__().__gt__(y)
SimulationCell.__ge__ = lambda self, y: self.__array__impl__().__ge__(y)
SimulationCell.__nonzero__ = lambda self: self.__array__impl__().__nonzero__()
SimulationCell.__neg__ = lambda self: self.__array__impl__().__neg__()
SimulationCell.__pos__ = lambda self: self.__array__impl__().__pos__()
SimulationCell.__abs__ = lambda self: self.__array__impl__().__abs__()
SimulationCell.__invert__ = lambda self: self.__array__impl__().__invert__()
SimulationCell.__add__ = lambda self, y: self.__array__impl__().__add__(y)
SimulationCell.__sub__ = lambda self, y: self.__array__impl__().__sub__(y)
SimulationCell.__mul__ = lambda self, y: self.__array__impl__().__mul__(y)
SimulationCell.__div__ = lambda self, y: self.__array__impl__().__div__(y)
SimulationCell.__truediv__ = lambda self, y: self.__array__impl__().__truediv__(y)
SimulationCell.__floordiv__ = lambda self, y: self.__array__impl__().__floordiv__(y)
SimulationCell.__mod__ = lambda self, y: self.__array__impl__().__mod__(y)
SimulationCell.__pow__ = lambda self, y: self.__array__impl__().__pow__(y)
SimulationCell.__and__ = lambda self, y: self.__array__impl__().__and__(y)
SimulationCell.__or__ = lambda self, y: self.__array__impl__().__or__(y)
SimulationCell.__xor__ = lambda self, y: self.__array__impl__().__xor__(y)

# Augmented arithmetic assignment operators, which require write access to the array:
def __SimulationCell__iadd__(self, y):
    with self: self.__array__impl__().__iadd__(y)
    return self
SimulationCell.__iadd__ = __SimulationCell__iadd__

def __SimulationCell__isub__(self, y):
    with self: self.__array__impl__().__isub__(y)
    return self
SimulationCell.__isub__ = __SimulationCell__isub__

def __SimulationCell__imul__(self, y):
    with self: self.__array__impl__().__imul__(y)
    return self
SimulationCell.__imul__ = __SimulationCell__imul__

def __SimulationCell__idiv__(self, y):
    with self: self.__array__impl__().__idiv__(y)
    return self
SimulationCell.__idiv__ = __SimulationCell__idiv__

def __SimulationCell__itruediv__(self, y):
    with self: self.__array__impl__().__itruediv__(y)
    return self
SimulationCell.__itruediv__ = __SimulationCell__itruediv__

def __SimulationCell__ifloordiv__(self, y):
    with self: self.__array__impl__().__ifloordiv__(y)
    return self
SimulationCell.__ifloordiv__ = __SimulationCell__ifloordiv__

def __SimulationCell__imod__(self, y):
    with self: self.__array__impl__().__imod__(y)
    return self
SimulationCell.__imod__ = __SimulationCell__imod__

def __SimulationCell__ipow__(self, y):
    with self: self.__array__impl__().__ipow__(y)
    return self
SimulationCell.__ipow__ = __SimulationCell__ipow__

def __SimulationCell__iand__(self, y):
    with self: self.__array__impl__().__iand__(y)
    return self
SimulationCell.__iand__ = __SimulationCell__iand__

def __SimulationCell__ior__(self, y):
    with self: self.__array__impl__().__ior__(y)
    return self
SimulationCell.__ior__ = __SimulationCell__ior__

def __SimulationCell__ixor__(self, y):
    with self: self.__array__impl__().__ixor__(y)
    return self
SimulationCell.__ixor__ = __SimulationCell__ixor__

# Implement SimulationCell.ndim attribute.
SimulationCell.ndim = property(lambda self: 2)

# Implement SimulationCell.shape attribute.
SimulationCell.shape = property(lambda self: (3, 4))

# Implement 'dtype' attribute.
SimulationCell.dtype = property(lambda self: self.__array__impl__().dtype)

# Context manager section entry method:
def _SimulationCell__enter__(self):
    self.make_writable()
    return self
SimulationCell.__enter__ = _SimulationCell__enter__

# Context manager section exit method:
def _SimulationCell__exit__(self, exc_type, exc_value, traceback):
    self.make_readonly()
    self.notify_object_changed()
    return False
SimulationCell.__exit__ = _SimulationCell__exit__

# Implementation of the SimulationCell.pbc property.
def _get_SimulationCell_pbc(self):
    """ A tuple of three boolean values, which specify periodic boundary flags of the simulation cell along each cell vector. """
    return (self.pbc_x, self.pbc_y, self.pbc_z)

def _set_SimulationCell_pbc(self, flags):
    assert(len(flags) == 3) # Expected an array with three Boolean flags.
    self.pbc_x = flags[0]
    self.pbc_y = flags[1]
    self.pbc_z = flags[2]

SimulationCell.pbc = property(_get_SimulationCell_pbc, _set_SimulationCell_pbc)
