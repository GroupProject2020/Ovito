import numpy

# Load dependencies
import ovito

# Load the native code module
from ovito.plugins.StdObj import Property

# Implement indexing for properties.
Property.__getitem__ = lambda self, idx: numpy.asanyarray(self)[idx]

def _Property__setitem__(self, idx, value): 
    numpy.asanyarray(self)[idx] = value
Property.__setitem__ = _Property__setitem__

# Implement iteration.
Property.__iter__ = lambda self: iter(numpy.asanyarray(self))

# Operator overloading.
Property.__eq__ = lambda self, y: numpy.asanyarray(self).__eq__(y)
Property.__ne__ = lambda self, y: numpy.asanyarray(self).__ne__(y)
Property.__lt__ = lambda self, y: numpy.asanyarray(self).__lt__(y)
Property.__le__ = lambda self, y: numpy.asanyarray(self).__le__(y)
Property.__gt__ = lambda self, y: numpy.asanyarray(self).__gt__(y)
Property.__ge__ = lambda self, y: numpy.asanyarray(self).__ge__(y)
Property.__nonzero__ = lambda self: numpy.asanyarray(self).__nonzero__()
Property.__neg__ = lambda self: numpy.asanyarray(self).__neg__()
Property.__pos__ = lambda self: numpy.asanyarray(self).__pos__()
Property.__abs__ = lambda self: numpy.asanyarray(self).__abs__()
Property.__invert__ = lambda self: numpy.asanyarray(self).__invert__()
Property.__add__ = lambda self, y: numpy.asanyarray(self).__add__(y)
Property.__sub__ = lambda self, y: numpy.asanyarray(self).__sub__(y)
Property.__mul__ = lambda self, y: numpy.asanyarray(self).__mul__(y)
Property.__div__ = lambda self, y: numpy.asanyarray(self).__div__(y)
Property.__truediv__ = lambda self, y: numpy.asanyarray(self).__truediv__(y)
Property.__floordiv__ = lambda self, y: numpy.asanyarray(self).__floordiv__(y)
Property.__mod__ = lambda self, y: numpy.asanyarray(self).__mod__(y)
Property.__pow__ = lambda self, y: numpy.asanyarray(self).__pow__(y)
Property.__and__ = lambda self, y: numpy.asanyarray(self).__and__(y)
Property.__or__ = lambda self, y: numpy.asanyarray(self).__or__(y)
Property.__xor__ = lambda self, y: numpy.asanyarray(self).__xor__(y)
Property.__iadd__ = lambda self, y: numpy.asanyarray(self).__iadd__(y)
Property.__isub__ = lambda self, y: numpy.asanyarray(self).__isub__(y)
Property.__imul__ = lambda self, y: numpy.asanyarray(self).__imul__(y)
Property.__idiv__ = lambda self, y: numpy.asanyarray(self).__idiv__(y)
Property.__itruediv__ = lambda self, y: numpy.asanyarray(self).__itruediv__(y)
Property.__ifloordiv__ = lambda self, y: numpy.asanyarray(self).__ifloordiv__(y)
Property.__imod__ = lambda self, y: numpy.asanyarray(self).__imod__(y)
Property.__ipow__ = lambda self, y: numpy.asanyarray(self).__ipow__(y)
Property.__iand__ = lambda self, y: numpy.asanyarray(self).__iand__(y)
Property.__ior__ = lambda self, y: numpy.asanyarray(self).__ior__(y)
Property.__ixor__ = lambda self, y: numpy.asanyarray(self).__ixor__(y)

# Printing / string representation
Property.__repr__ = lambda self: self.__class__.__name__ + "('" + self.name + "')"

# Implement 'ndim' attribute.
def _Property_ndim(self):
    if self.components <= 1: return 1
    else: return 2
Property.ndim = property(_Property_ndim)

# Implement 'shape' attribute.
def _Property_shape(self):
    if self.components <= 1: return (len(self), ) 
    else: return (len(self), self.components)
Property.shape = property(_Property_shape)

# Implement 'dtype' attribute.
def _Property_dtype(self):
    return numpy.asanyarray(self).dtype
Property.dtype = property(_Property_dtype)

# Context manager entry method:
def _Property__enter__(self):
    self.make_writable()
    return numpy.asanyarray(self)
Property.__enter__ = _Property__enter__

# Context manager exit method:
def _Property__exit__(self, exc_type, exc_value, traceback):
    self.make_readonly()
    self.notify_object_changed()
    return False
Property.__exit__ = _Property__exit__

# Implementation of Property.modify() method.
# For backward compatibility with OVITO 3.0.0:
def _Property_modify(self):
    # Creates a Python context manager that manages write access to the internal property values.
    # The manager returns a mutable Numpy array when used in a Python ``with`` statement,
    # which provides a read/write view of the internal data.

    # A Python context manager for managing write access to the internal property data.
    # It returns a writable Numpy array when used in a Python 'with' statement.
    # Upon exit of the runtime context, it automatically calls Property.notify_object_changed()
    # to inform the pipeline system of the changed property data.
    class _PropertyModificationManager:

        def __init__(self, owner):
            """ Constructor that stores away a back-pointer to the owning property object. """
            self._owner = owner

        def __enter__(self):
            """ Enter the runtime context related to this object. """
            self._owner.make_writable()
            return numpy.asanyarray(self._owner)

        def __exit__(self, exc_type, exc_value, traceback):
            """ Exit the runtime context related to this object. """
            self._owner.make_readonly()
            self._owner.notify_object_changed()
            return False

    return _PropertyModificationManager(self)

Property.modify = _Property_modify


# Returns a NumPy array wrapper for a property.
# For backward compatibility with OVITO 2.9.0:
def _Property_array(self):
    # This attribute returns a NumPy array, which provides read access to the per-element data stored in this property object.
    return numpy.asanyarray(self)
Property.array = property(_Property_array)

# Returns a NumPy array wrapper for a property with write access.
# For backward compatibility with OVITO 2.9.0:
def _Property_marray(self):
    self.make_writable()
    a = numpy.asanyarray(self)
    self.make_readonly()
    return a

# This is needed to enable the augmented assignment operators (+=, -=, etc.) for the 'marray' property.
# For backward compatibility with OVITO 2.9.0:
def _Property_marray_assign(self, other):
    if not hasattr(other, "__array_interface__"):
        raise ValueError("Only objects supporting the array interface can be assigned to the 'marray' property.")
    o = other.__array_interface__
    s = self.__array_interface__
    if o["shape"] != s["shape"] or o["typestr"] != s["typestr"] or o["data"] != s["data"]:
        raise ValueError("Assignment to the 'marray' property is restricted. Left and right-hand side must be identical.")
    # Assume that the data has been changed in the meantime.
    self.notify_object_changed()
        
# For backward compatibility with OVITO 2.9.0:
Property.marray = property(_Property_marray, _Property_marray_assign)