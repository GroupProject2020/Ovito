import numpy

# Load dependencies
import ovito

# Load the native code module
from ..plugins.PyScript import Property

# Implement indexing for properties.
Property.__getitem__ = lambda self, idx: numpy.asanyarray(self)[idx]

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

# Implementation of Property.modify() method:
def _Property_modify(self):
    """
    Creates a Python context manager that manages write access to the internal property values.
    The manager returns a mutable Numpy array when used in a Python ``with`` statement,
    which provides a read/write view of the internal data.
    """

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
def _Property_marray_assign(self, other):
    if not hasattr(other, "__array_interface__"):
        raise ValueError("Only objects supporting the array interface can be assigned to the 'marray' property.")
    o = other.__array_interface__
    s = self.__mutable_array_interface__
    if o["shape"] != s["shape"] or o["typestr"] != s["typestr"] or o["data"] != s["data"]:
        raise ValueError("Assignment to the 'marray' property is restricted. Left and right-hand side must be identical.")
    # Assume that the data has been changed in the meantime.
    self.notify_object_changed()
        
# For backward compatibility with OVITO 2.9.0:
Property.marray = property(_Property_marray, _Property_marray_assign)