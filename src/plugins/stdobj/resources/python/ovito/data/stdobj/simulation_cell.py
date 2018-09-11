# Load dependencies
import ovito
import numpy

# Load the native code module
from ovito.plugins.StdObj import SimulationCell

# Implement indexing for simulation cell objects.
SimulationCell.__getitem__ = lambda self, idx: numpy.asanyarray(self)[idx]

def _SimulationCell__setitem__(self, idx, value): 
    numpy.asanyarray(self)[idx] = value
SimulationCell.__setitem__ = _SimulationCell__setitem__

# Implement iteration for simulation cell objects.
SimulationCell.__iter__ = lambda self: iter(numpy.asanyarray(self))

# Operator overloading.
SimulationCell.__eq__ = lambda self, y: numpy.asanyarray(self).__eq__(y)
SimulationCell.__ne__ = lambda self, y: numpy.asanyarray(self).__ne__(y)
SimulationCell.__lt__ = lambda self, y: numpy.asanyarray(self).__lt__(y)
SimulationCell.__le__ = lambda self, y: numpy.asanyarray(self).__le__(y)
SimulationCell.__gt__ = lambda self, y: numpy.asanyarray(self).__gt__(y)
SimulationCell.__ge__ = lambda self, y: numpy.asanyarray(self).__ge__(y)
SimulationCell.__nonzero__ = lambda self: numpy.asanyarray(self).__nonzero__()
SimulationCell.__neg__ = lambda self: numpy.asanyarray(self).__neg__()
SimulationCell.__pos__ = lambda self: numpy.asanyarray(self).__pos__()
SimulationCell.__abs__ = lambda self: numpy.asanyarray(self).__abs__()
SimulationCell.__invert__ = lambda self: numpy.asanyarray(self).__invert__()
SimulationCell.__add__ = lambda self, y: numpy.asanyarray(self).__add__(y)
SimulationCell.__sub__ = lambda self, y: numpy.asanyarray(self).__sub__(y)
SimulationCell.__mul__ = lambda self, y: numpy.asanyarray(self).__mul__(y)
SimulationCell.__div__ = lambda self, y: numpy.asanyarray(self).__div__(y)
SimulationCell.__truediv__ = lambda self, y: numpy.asanyarray(self).__truediv__(y)
SimulationCell.__floordiv__ = lambda self, y: numpy.asanyarray(self).__floordiv__(y)
SimulationCell.__mod__ = lambda self, y: numpy.asanyarray(self).__mod__(y)
SimulationCell.__pow__ = lambda self, y: numpy.asanyarray(self).__pow__(y)
SimulationCell.__and__ = lambda self, y: numpy.asanyarray(self).__and__(y)
SimulationCell.__or__ = lambda self, y: numpy.asanyarray(self).__or__(y)
SimulationCell.__xor__ = lambda self, y: numpy.asanyarray(self).__xor__(y)
SimulationCell.__iadd__ = lambda self, y: numpy.asanyarray(self).__iadd__(y)
SimulationCell.__isub__ = lambda self, y: numpy.asanyarray(self).__isub__(y)
SimulationCell.__imul__ = lambda self, y: numpy.asanyarray(self).__imul__(y)
SimulationCell.__idiv__ = lambda self, y: numpy.asanyarray(self).__idiv__(y)
SimulationCell.__itruediv__ = lambda self, y: numpy.asanyarray(self).__itruediv__(y)
SimulationCell.__ifloordiv__ = lambda self, y: numpy.asanyarray(self).__ifloordiv__(y)
SimulationCell.__imod__ = lambda self, y: numpy.asanyarray(self).__imod__(y)
SimulationCell.__ipow__ = lambda self, y: numpy.asanyarray(self).__ipow__(y)
SimulationCell.__iand__ = lambda self, y: numpy.asanyarray(self).__iand__(y)
SimulationCell.__ior__ = lambda self, y: numpy.asanyarray(self).__ior__(y)
SimulationCell.__ixor__ = lambda self, y: numpy.asanyarray(self).__ixor__(y)

# Implement SimulationCell.ndim attribute.
SimulationCell.ndim = property(lambda self: 2)

# Implement SimulationCell.shape attribute.
SimulationCell.shape = property(lambda self: (3, 4))

# Implement 'dtype' attribute.
SimulationCell.dtype = property(lambda self: numpy.asanyarray(self).dtype)

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

# Context manager section entry method:
def _SimulationCell__enter__(self):
    self.make_writable()
    return numpy.asanyarray(self)
SimulationCell.__enter__ = _SimulationCell__enter__

# Context manager section exit method:
def _SimulationCell__exit__(self, exc_type, exc_value, traceback):
    self.make_readonly()
    self.notify_object_changed()
    return False
SimulationCell.__exit__ = _SimulationCell__exit__