import numpy

# Load dependencies.
import ovito

# Load the native code module.
from ovito.plugins.StdObjPython import Property

# View into a property array, which manages write access to the underlying data.
class PropertyView(numpy.ndarray):
    def __new__(cls, input_array, ovito_property):
        # Input array is an already formed ndarray instance. We first cast to be our class type.
        obj = input_array.view(cls)
        # Keep a reference to the underlying OVITO object managing the memory.
        obj._data_context_manager = ovito_property
        # Finally, return the newly created object.
        return obj

    def __array_finalize__(self, obj):
        self._data_context_manager = self
        if obj is None: return
        # Do not keep a reference to the underlying OVITO object if this NumPy array manages its own memory (independent of OVITO).
        # To find out, we need to walk up the chain of base objects.
        b = self
        while isinstance(b, numpy.ndarray):
            if b.flags.owndata: return
            b = b.base
        self._data_context_manager = getattr(obj, '_data_context_manager', self)

    # Implement a no-op context manager.
    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        return False

    # Indexed assignment.
    def __setitem__(self, idx, value):
        with self._data_context_manager: super().__setitem__(idx, value)

    # Augmented arithmetic assignment operators, which require write access to the array:
    def __iadd__(self, y):
        with self._data_context_manager: return super().__iadd__(y)
    def __isub__(self, y):
        with self._data_context_manager: return super().__isub__(y)
    def __imul__(self, y):
        with self._data_context_manager: return super().__imul__(y)
    def __idiv__(self, y):
        with self._data_context_manager: return super().__idiv__(y)
    def __itruediv__(self, y):
        with self._data_context_manager: return super().__itruediv__(y)
    def __ifloordiv__(self, y):
        with self._data_context_manager: return super().__ifloordiv__(y)
    def __imod__(self, y):
        with self._data_context_manager: return super().__imod__(y)
    def __ipow__(self, y):
        with self._data_context_manager: return super().__ipow__(y)
    def __iand__(self, y):
        with self._data_context_manager: return super().__iand__(y)
    def __ior__(self, y):
        with self._data_context_manager: return super().__ior__(y)
    def __ixor__(self, y):
        with self._data_context_manager: return super().__ixor__(y)

# Implement indexing for Property arrays.
def _Property__getitem__(self, idx):
    return self.__array__()[idx]
Property.__getitem__ = _Property__getitem__

# Indexed assignment.
def _Property__setitem__(self, idx, value):
    # Write access to a property array requires a 'with' Python statement.
    with self: self.__array__impl__()[idx] = value
Property.__setitem__ = _Property__setitem__

# Implementation of the Numpy array protocol:
def _Property__array__(self, dtype=None):
    return PropertyView(self.__array__impl__(dtype), self)
Property.__array__ = _Property__array__

# Array element iteration.
Property.__iter__ = lambda self: iter(self.__array__())

# Standard array operators.
Property.__eq__ = lambda self, y: self.__array__impl__().__eq__(y)
Property.__ne__ = lambda self, y: self.__array__impl__().__ne__(y)
Property.__lt__ = lambda self, y: self.__array__impl__().__lt__(y)
Property.__le__ = lambda self, y: self.__array__impl__().__le__(y)
Property.__gt__ = lambda self, y: self.__array__impl__().__gt__(y)
Property.__ge__ = lambda self, y: self.__array__impl__().__ge__(y)
Property.__nonzero__ = lambda self: self.__array__impl__().__nonzero__()
Property.__neg__ = lambda self: self.__array__impl__().__neg__()
Property.__pos__ = lambda self: self.__array__impl__().__pos__()
Property.__abs__ = lambda self: self.__array__impl__().__abs__()
Property.__invert__ = lambda self: self.__array__impl__().__invert__()
Property.__add__ = lambda self, y: self.__array__impl__().__add__(y)
Property.__sub__ = lambda self, y: self.__array__impl__().__sub__(y)
Property.__mul__ = lambda self, y: self.__array__impl__().__mul__(y)
Property.__div__ = lambda self, y: self.__array__impl__().__div__(y)
Property.__truediv__ = lambda self, y: self.__array__impl__().__truediv__(y)
Property.__floordiv__ = lambda self, y: self.__array__impl__().__floordiv__(y)
Property.__mod__ = lambda self, y: self.__array__impl__().__mod__(y)
Property.__pow__ = lambda self, y: self.__array__impl__().__pow__(y)
Property.__and__ = lambda self, y: self.__array__impl__().__and__(y)
Property.__or__ = lambda self, y: self.__array__impl__().__or__(y)
Property.__xor__ = lambda self, y: self.__array__impl__().__xor__(y)

# Augmented arithmetic assignment operators, which require write access to the array:
def __Property__iadd__(self, y):
    with self: self.__array__impl__().__iadd__(y)
    return self
Property.__iadd__ = __Property__iadd__

def __Property__isub__(self, y):
    with self: self.__array__impl__().__isub__(y)
    return self
Property.__isub__ = __Property__isub__

def __Property__imul__(self, y):
    with self: self.__array__impl__().__imul__(y)
    return self
Property.__imul__ = __Property__imul__

def __Property__idiv__(self, y):
    with self: self.__array__impl__().__idiv__(y)
    return self
Property.__idiv__ = __Property__idiv__

def __Property__itruediv__(self, y):
    with self: self.__array__impl__().__itruediv__(y)
    return self
Property.__itruediv__ = __Property__itruediv__

def __Property__ifloordiv__(self, y):
    with self: self.__array__impl__().__ifloordiv__(y)
    return self
Property.__ifloordiv__ = __Property__ifloordiv__

def __Property__imod__(self, y):
    with self: self.__array__impl__().__imod__(y)
    return self
Property.__imod__ = __Property__imod__

def __Property__ipow__(self, y):
    with self: self.__array__impl__().__ipow__(y)
    return self
Property.__ipow__ = __Property__ipow__

def __Property__iand__(self, y):
    with self: self.__array__impl__().__iand__(y)
    return self
Property.__iand__ = __Property__iand__

def __Property__ior__(self, y):
    with self: self.__array__impl__().__ior__(y)
    return self
Property.__ior__ = __Property__ior__

def __Property__ixor__(self, y):
    with self: self.__array__impl__().__ixor__(y)
    return self
Property.__ixor__ = __Property__ixor__

# Printing / string representation.
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
    return self.__array__impl__().dtype
Property.dtype = property(_Property_dtype)

# Implement 'T' attribute (array transposition).
def _Property_T(self):
    return self.__array__().T
Property.T = property(_Property_T)

# Context manager section entry method:
def _Property__enter__(self):
    self.make_writable()
    return self
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
