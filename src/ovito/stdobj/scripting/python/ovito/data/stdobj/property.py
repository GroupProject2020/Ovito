import numpy

# Load dependencies
import ovito

# Load the native code module
from ovito.plugins.StdObjPython import Property

# Implement indexing for properties.
Property.__getitem__ = lambda self, idx: self.__array__()[idx]

def _Property__setitem__(self, idx, value):
    self.__array__()[idx] = value  # Note to users: Write access to a property requires a 'with' Python statement.
Property.__setitem__ = _Property__setitem__

# Implement iteration.
Property.__iter__ = lambda self: iter(self.__array__())

# Operator overloading.
Property.__eq__ = lambda self, y: self.__array__().__eq__(y)
Property.__ne__ = lambda self, y: self.__array__().__ne__(y)
Property.__lt__ = lambda self, y: self.__array__().__lt__(y)
Property.__le__ = lambda self, y: self.__array__().__le__(y)
Property.__gt__ = lambda self, y: self.__array__().__gt__(y)
Property.__ge__ = lambda self, y: self.__array__().__ge__(y)
Property.__nonzero__ = lambda self: self.__array__().__nonzero__()
Property.__neg__ = lambda self: self.__array__().__neg__()
Property.__pos__ = lambda self: self.__array__().__pos__()
Property.__abs__ = lambda self: self.__array__().__abs__()
Property.__invert__ = lambda self: self.__array__().__invert__()
Property.__add__ = lambda self, y: self.__array__().__add__(y)
Property.__sub__ = lambda self, y: self.__array__().__sub__(y)
Property.__mul__ = lambda self, y: self.__array__().__mul__(y)
Property.__div__ = lambda self, y: self.__array__().__div__(y)
Property.__truediv__ = lambda self, y: self.__array__().__truediv__(y)
Property.__floordiv__ = lambda self, y: self.__array__().__floordiv__(y)
Property.__mod__ = lambda self, y: self.__array__().__mod__(y)
Property.__pow__ = lambda self, y: self.__array__().__pow__(y)
Property.__and__ = lambda self, y: self.__array__().__and__(y)
Property.__or__ = lambda self, y: self.__array__().__or__(y)
Property.__xor__ = lambda self, y: self.__array__().__xor__(y)
Property.__iadd__ = lambda self, y: self.__array__().__iadd__(y)
Property.__isub__ = lambda self, y: self.__array__().__isub__(y)
Property.__imul__ = lambda self, y: self.__array__().__imul__(y)
Property.__idiv__ = lambda self, y: self.__array__().__idiv__(y)
Property.__itruediv__ = lambda self, y: self.__array__().__itruediv__(y)
Property.__ifloordiv__ = lambda self, y: self.__array__().__ifloordiv__(y)
Property.__imod__ = lambda self, y: self.__array__().__imod__(y)
Property.__ipow__ = lambda self, y: self.__array__().__ipow__(y)
Property.__iand__ = lambda self, y: self.__array__().__iand__(y)
Property.__ior__ = lambda self, y: self.__array__().__ior__(y)
Property.__ixor__ = lambda self, y: self.__array__().__ixor__(y)

# Printing / string representation
Property.__repr__ = lambda self: self.__class__.__name__ + "('" + self.name + "')"

# Implement 'ndim' attribute.
def _Property_ndim(self):
    if self.component_count <= 1: return 1
    else: return 2
Property.ndim = property(_Property_ndim)

# Implement 'shape' attribute.
def _Property_shape(self):
    if self.component_count <= 1: return (len(self), )
    else: return (len(self), self.component_count)
Property.shape = property(_Property_shape)

# Implement 'dtype' attribute.
def _Property_dtype(self):
    return self.__array__().dtype
Property.dtype = property(_Property_dtype)

# Implement 'T' attribute (array transposition).
def _Property_T(self):
    return self.__array__().T
Property.T = property(_Property_T)

# Context manager section entry method:
def _Property__enter__(self):
    self.make_writable()
    return self.__array__()
Property.__enter__ = _Property__enter__

# Context manager section exit method:
def _Property__exit__(self, exc_type, exc_value, traceback):
    self.make_readonly()
    self.notify_object_changed()
    return False
Property.__exit__ = _Property__exit__

# Returns a NumPy array wrapper for a property.
# For backward compatibility with OVITO 2.9.0:
def _Property_array(self):
    # This attribute returns a NumPy array, which provides read access to the per-element data stored in this property object.
    return self.__array__()
Property.array = property(_Property_array)

# Returns a NumPy array wrapper for a property with write access.
# For backward compatibility with OVITO 2.9.0:
def _Property_marray(self):
    self.make_writable()
    a = self.__array__()
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

def _Property_type_by_id(self, id):
    """
    Looks up the :py:class:`ElementType` with the given numeric ID in the :py:attr:`.types` list.
    Raises a ``KeyError`` if the ID does not exist.
    """
    t = self._get_type_by_id(int(id))
    if t is None:
        raise KeyError("Element type with ID %i is not defined." % id)
    return t
Property.type_by_id = _Property_type_by_id

def _Property_type_by_name(self, name):
    """
    Looks up the :py:class:`ElementType` with the given name in the :py:attr:`.types` list.
    If multiple types exists with the same name, the first type is returned.
    Raises a ``KeyError`` if there is no type with such a name.
    """
    t = self._get_type_by_name(name)
    if t is None:
        raise KeyError("Element type with name '%s' is not defined." % name)
    return t
Property.type_by_name = _Property_type_by_name

# For backward compatibility with OVITO 2.9.0:
Property.marray = property(_Property_marray, _Property_marray_assign)
Property.get_type_by_id = lambda self, id: self.type_by_id(id)
Property.get_type_by_name = lambda self, name: self.type_by_name(name)
Property.type_list = property(lambda self: self.types)
