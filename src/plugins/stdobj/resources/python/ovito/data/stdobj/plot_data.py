import numpy

# Load dependencies
import ovito

# Load the native code module
from ovito.plugins.StdObj import PlotData

# Implement indexing for plot objects.
PlotData.__getitem__ = lambda self, idx: numpy.asanyarray(self)[idx]

# Implement iteration.
PlotData.__iter__ = lambda self: iter(numpy.asanyarray(self))

# Operator overloading.
PlotData.__eq__ = lambda self, y: numpy.asanyarray(self).__eq__(y)
PlotData.__ne__ = lambda self, y: numpy.asanyarray(self).__ne__(y)
PlotData.__lt__ = lambda self, y: numpy.asanyarray(self).__lt__(y)
PlotData.__le__ = lambda self, y: numpy.asanyarray(self).__le__(y)
PlotData.__gt__ = lambda self, y: numpy.asanyarray(self).__gt__(y)
PlotData.__ge__ = lambda self, y: numpy.asanyarray(self).__ge__(y)
PlotData.__nonzero__ = lambda self: numpy.asanyarray(self).__nonzero__()
PlotData.__neg__ = lambda self: numpy.asanyarray(self).__neg__()
PlotData.__pos__ = lambda self: numpy.asanyarray(self).__pos__()
PlotData.__abs__ = lambda self: numpy.asanyarray(self).__abs__()
PlotData.__invert__ = lambda self: numpy.asanyarray(self).__invert__()
PlotData.__add__ = lambda self, y: numpy.asanyarray(self).__add__(y)
PlotData.__sub__ = lambda self, y: numpy.asanyarray(self).__sub__(y)
PlotData.__mul__ = lambda self, y: numpy.asanyarray(self).__mul__(y)
PlotData.__div__ = lambda self, y: numpy.asanyarray(self).__div__(y)
PlotData.__truediv__ = lambda self, y: numpy.asanyarray(self).__truediv__(y)
PlotData.__floordiv__ = lambda self, y: numpy.asanyarray(self).__floordiv__(y)
PlotData.__mod__ = lambda self, y: numpy.asanyarray(self).__mod__(y)
PlotData.__pow__ = lambda self, y: numpy.asanyarray(self).__pow__(y)
PlotData.__and__ = lambda self, y: numpy.asanyarray(self).__and__(y)
PlotData.__or__ = lambda self, y: numpy.asanyarray(self).__or__(y)
PlotData.__xor__ = lambda self, y: numpy.asanyarray(self).__xor__(y)
PlotData.__iadd__ = lambda self, y: numpy.asanyarray(self).__iadd__(y)
PlotData.__isub__ = lambda self, y: numpy.asanyarray(self).__isub__(y)
PlotData.__imul__ = lambda self, y: numpy.asanyarray(self).__imul__(y)
PlotData.__idiv__ = lambda self, y: numpy.asanyarray(self).__idiv__(y)
PlotData.__itruediv__ = lambda self, y: numpy.asanyarray(self).__itruediv__(y)
PlotData.__ifloordiv__ = lambda self, y: numpy.asanyarray(self).__ifloordiv__(y)
PlotData.__imod__ = lambda self, y: numpy.asanyarray(self).__imod__(y)
PlotData.__ipow__ = lambda self, y: numpy.asanyarray(self).__ipow__(y)
PlotData.__iand__ = lambda self, y: numpy.asanyarray(self).__iand__(y)
PlotData.__ior__ = lambda self, y: numpy.asanyarray(self).__ior__(y)
PlotData.__ixor__ = lambda self, y: numpy.asanyarray(self).__ixor__(y)

# Printing / string representation
PlotData.__repr__ = lambda self: self.__class__.__name__ + "('" + self.title + "')"

# Implement 'ndim' attribute.
def _PlotData_ndim(self):
    return numpy.asanyarray(self).ndim
PlotData.ndim = property(_PlotData_ndim)

# Implement 'shape' attribute.
def _PlotData_shape(self):
    return numpy.asanyarray(self).shape
PlotData.shape = property(_PlotData_shape)

# Implement 'dtype' attribute.
def _PlotData_dtype(self):
    return numpy.asanyarray(self).dtype
PlotData.dtype = property(_PlotData_dtype)

# Implement 'T' attribute (array transposition).
def _PlotData_T(self):
    return numpy.asanyarray(self).T
PlotData.T = property(_PlotData_T)
