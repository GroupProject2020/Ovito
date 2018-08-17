import numpy

# Load dependencies
import ovito

# Load the native code module
from ovito.plugins.StdObj import DataSeries

# Implement indexing for data series objects.
DataSeries.__getitem__ = lambda self, idx: numpy.asanyarray(self)[idx]

# Implement iteration.
DataSeries.__iter__ = lambda self: iter(numpy.asanyarray(self))

# Operator overloading.
DataSeries.__eq__ = lambda self, y: numpy.asanyarray(self).__eq__(y)
DataSeries.__ne__ = lambda self, y: numpy.asanyarray(self).__ne__(y)
DataSeries.__lt__ = lambda self, y: numpy.asanyarray(self).__lt__(y)
DataSeries.__le__ = lambda self, y: numpy.asanyarray(self).__le__(y)
DataSeries.__gt__ = lambda self, y: numpy.asanyarray(self).__gt__(y)
DataSeries.__ge__ = lambda self, y: numpy.asanyarray(self).__ge__(y)
DataSeries.__nonzero__ = lambda self: numpy.asanyarray(self).__nonzero__()
DataSeries.__neg__ = lambda self: numpy.asanyarray(self).__neg__()
DataSeries.__pos__ = lambda self: numpy.asanyarray(self).__pos__()
DataSeries.__abs__ = lambda self: numpy.asanyarray(self).__abs__()
DataSeries.__invert__ = lambda self: numpy.asanyarray(self).__invert__()
DataSeries.__add__ = lambda self, y: numpy.asanyarray(self).__add__(y)
DataSeries.__sub__ = lambda self, y: numpy.asanyarray(self).__sub__(y)
DataSeries.__mul__ = lambda self, y: numpy.asanyarray(self).__mul__(y)
DataSeries.__div__ = lambda self, y: numpy.asanyarray(self).__div__(y)
DataSeries.__truediv__ = lambda self, y: numpy.asanyarray(self).__truediv__(y)
DataSeries.__floordiv__ = lambda self, y: numpy.asanyarray(self).__floordiv__(y)
DataSeries.__mod__ = lambda self, y: numpy.asanyarray(self).__mod__(y)
DataSeries.__pow__ = lambda self, y: numpy.asanyarray(self).__pow__(y)
DataSeries.__and__ = lambda self, y: numpy.asanyarray(self).__and__(y)
DataSeries.__or__ = lambda self, y: numpy.asanyarray(self).__or__(y)
DataSeries.__xor__ = lambda self, y: numpy.asanyarray(self).__xor__(y)
DataSeries.__iadd__ = lambda self, y: numpy.asanyarray(self).__iadd__(y)
DataSeries.__isub__ = lambda self, y: numpy.asanyarray(self).__isub__(y)
DataSeries.__imul__ = lambda self, y: numpy.asanyarray(self).__imul__(y)
DataSeries.__idiv__ = lambda self, y: numpy.asanyarray(self).__idiv__(y)
DataSeries.__itruediv__ = lambda self, y: numpy.asanyarray(self).__itruediv__(y)
DataSeries.__ifloordiv__ = lambda self, y: numpy.asanyarray(self).__ifloordiv__(y)
DataSeries.__imod__ = lambda self, y: numpy.asanyarray(self).__imod__(y)
DataSeries.__ipow__ = lambda self, y: numpy.asanyarray(self).__ipow__(y)
DataSeries.__iand__ = lambda self, y: numpy.asanyarray(self).__iand__(y)
DataSeries.__ior__ = lambda self, y: numpy.asanyarray(self).__ior__(y)
DataSeries.__ixor__ = lambda self, y: numpy.asanyarray(self).__ixor__(y)

# Printing / string representation
DataSeries.__repr__ = lambda self: self.__class__.__name__ + "('" + self.id + "')"

# Implement 'ndim' attribute.
def _DataSeries_ndim(self):
    return numpy.asanyarray(self).ndim
DataSeries.ndim = property(_DataSeries_ndim)

# Implement 'shape' attribute.
def _DataSeries_shape(self):
    return numpy.asanyarray(self).shape
DataSeries.shape = property(_DataSeries_shape)

# Implement 'dtype' attribute.
def _DataSeries_dtype(self):
    return numpy.asanyarray(self).dtype
DataSeries.dtype = property(_DataSeries_dtype)

# Implement 'T' attribute (array transposition).
def _DataSeries_T(self):
    return numpy.asanyarray(self).T
DataSeries.T = property(_DataSeries_T)

# Implement as_table() method.
def _DataSeries_as_table(self):
    x = self.x
    y = self.y
    if not x:
        half_step_size = 0.5 * (self.interval_end - self.interval_start) / len(y)
        x = numpy.linspace(half_step_size, self.interval_end - half_step_size, num = len(y))
    return numpy.vstack(numpy.atleast_2d(x, y))
DataSeries.as_table = _DataSeries_as_table
