import numpy

# Load dependencies
import ovito

# Load the native code module
from ..plugins.PyScript.Scene import PropertyObject

# Implement indexing for properties.
PropertyObject.__getitem__ = lambda self, idx: numpy.asanyarray(self)[idx]

# Implement iteration.
PropertyObject.__iter__ = lambda self: iter(numpy.asanyarray(self))

# Operator overloading.
PropertyObject.__eq__ = lambda self, y: numpy.asanyarray(self).__eq__(y)
PropertyObject.__ne__ = lambda self, y: numpy.asanyarray(self).__ne__(y)
PropertyObject.__lt__ = lambda self, y: numpy.asanyarray(self).__lt__(y)
PropertyObject.__le__ = lambda self, y: numpy.asanyarray(self).__le__(y)
PropertyObject.__gt__ = lambda self, y: numpy.asanyarray(self).__gt__(y)
PropertyObject.__ge__ = lambda self, y: numpy.asanyarray(self).__ge__(y)
PropertyObject.__nonzero__ = lambda self: numpy.asanyarray(self).__nonzero__()
PropertyObject.__neg__ = lambda self: numpy.asanyarray(self).__neg__()
PropertyObject.__pos__ = lambda self: numpy.asanyarray(self).__pos__()
PropertyObject.__abs__ = lambda self: numpy.asanyarray(self).__abs__()
PropertyObject.__invert__ = lambda self: numpy.asanyarray(self).__invert__()
PropertyObject.__add__ = lambda self, y: numpy.asanyarray(self).__add__(y)
PropertyObject.__sub__ = lambda self, y: numpy.asanyarray(self).__sub__(y)
PropertyObject.__mul__ = lambda self, y: numpy.asanyarray(self).__mul__(y)
PropertyObject.__div__ = lambda self, y: numpy.asanyarray(self).__div__(y)
PropertyObject.__truediv__ = lambda self, y: numpy.asanyarray(self).__truediv__(y)
PropertyObject.__floordiv__ = lambda self, y: numpy.asanyarray(self).__floordiv__(y)
PropertyObject.__mod__ = lambda self, y: numpy.asanyarray(self).__mod__(y)
PropertyObject.__pow__ = lambda self, y: numpy.asanyarray(self).__pow__(y)
PropertyObject.__and__ = lambda self, y: numpy.asanyarray(self).__and__(y)
PropertyObject.__or__ = lambda self, y: numpy.asanyarray(self).__or__(y)
PropertyObject.__xor__ = lambda self, y: numpy.asanyarray(self).__xor__(y)

# Implement 'ndim' attribute.
def _PropertyObject_ndim(self):
    if self.components <= 1: return 1
    else: return 2
PropertyObject.ndim = property(_PropertyObject_ndim)

# Implement 'shape' attribute.
def _PropertyObject_shape(self):
    if self.components <= 1: return (len(self), ) 
    else: return (len(self), self.components)
PropertyObject.shape = property(_PropertyObject_shape)

# Implementation of ParticleProperty.modify() method:
def _PropertyObject_modify(self):

    # A Python context manager for managing write access to the internal property data.
    # It returns a writable Numpy array when used in a Python 'with' statement.
    # Upon exit of the runtime context, it automatically calls PropertyObject.notify_object_changed()
    # to inform the pipeline system of the changed property data.
    class _PropertyModificationManager:

        def __init__(self, owner):
            """ Constructor that stores away a back-pointer to the owning property object. """
            self._owner = owner

        def __enter__(self):
            """ Enter the runtime context related to this object. """
            return numpy.asarray(self)

        def __exit__(self, exc_type, exc_value, traceback):
            """ Exit the runtime context related to this object. """
            self._owner.notify_object_changed()
            return False

        @property
        def __array_interface__(self):
            """ Implementation of the Python Array interface. """
            return self._owner.__mutable_array_interface__

    return _PropertyModificationManager(self)

PropertyObject.modify = _PropertyObject_modify


# Returns a NumPy array wrapper for a particle property.
# For backward compatibility with OVITO 2.9.0:
def _PropertyObject_array(self):
    # This attribute returns a NumPy array, which provides read access to the per-element data stored in this property object.
    return numpy.asanyarray(self)
PropertyObject.array = property(_PropertyObject_array)

# Returns a NumPy array wrapper for a property with write access.
# For backward compatibility with OVITO 2.9.0:
def _PropertyObject_marray(self):
    # This attribute returns a *mutable* NumPy array providing read/write access to the internal per-element data.
    class DummyClass:
        pass
    o = DummyClass()
    o.__array_interface__ = self.__mutable_array_interface__
    # Create reference to particle property object to keep it alive.
    o.__base_property = self        
    return numpy.asarray(o)

# This is needed to enable the augmented assignment operators (+=, -=, etc.) for the 'marray' property.
# For backward compatibility with OVITO 2.9.0:
def _PropertyObject_marray_assign(self, other):
    if not hasattr(other, "__array_interface__"):
        raise ValueError("Only objects supporting the array interface can be assigned to the 'marray' property.")
    o = other.__array_interface__
    s = self.__mutable_array_interface__
    if o["shape"] != s["shape"] or o["typestr"] != s["typestr"] or o["data"] != s["data"]:
        raise ValueError("Assignment to the 'marray' property is restricted. Left and right-hand side must be identical.")
    # Assume that the data has been changed in the meantime.
    self.notify_object_changed()
        
# For backward compatibility with OVITO 2.9.0:
PropertyObject.marray = property(_PropertyObject_marray, _PropertyObject_marray_assign)