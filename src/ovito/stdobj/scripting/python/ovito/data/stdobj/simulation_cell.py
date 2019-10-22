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

# Implementation of the SimulationCell.delta_vector() method.
def _SimulationCell_delta_vector(self, ra, rb, return_pbcvec=False):
    """
    Computes the correct vector connecting points :math:`r_a` and :math:`r_b` in a periodic simulation cell by applying the minimum image convention.

    The method starts by computing the 3d vector :math:`{\Delta} = r_b - r_a` for two points :math:`r_a` and :math:`r_b`, which may be located in different images
    of the periodic simulation cell. The `minimum image convention <https://en.wikipedia.org/wiki/Periodic_boundary_conditions>`_
    is then applied to obtain the new vector :math:`{\Delta'} = r_b' - r_a`, where the original point :math:`r_b` has been replaced by the periodic image
    :math:`r_b'` that is closest to :math:`r_a`, making the vector :math:`{\Delta'}` as short as possible (in reduced coordinate space).
    :math:`r_b'` is obtained by translating :math:`r_b` an integer number of times along each of the three cell directions:
    :math:`r_b' = r_b - H*n`, with :math:`H` being the 3x3 cell matrix and :math:`n` being a vector of three integers that are chosen by the
    method such that :math:`r_b'` is as close to :math:`r_a` as possible.

    Note that the periodic image convention is applied only along those cell directions for which
    periodic boundary conditions are enabled (see :py:attr:`pbc` property). For other directions
    no shifting is performed, i.e., the corresponding components of :math:`n = (n_x,n_y,n_z)` will always be zero.

    The method is able to compute the results for either an individual pair of input points or for two *arrays* of input points. In the latter case,
    i.e. if the input parameters *ra* and *rb* are both 2-D arrays of shape *Nx3*, the method returns a 2-D array containing
    *N* output vectors. This allows applying the minimum image convention to a large number of point pairs in one function call.

    The optional *return_pbcvec* flag makes the method return as an additional output the vector :math:`n` introduced above.
    The components of this vector specify the number of times the image point :math:`r_b'` needs to be shifted along each of the three cell directions
    in order to bring it onto the original input point :math:`r_b`. In other words, it specifies the number of times the
    computed vector :math:`{\Delta} = r_b - r_a` crosses a periodic boundary of the cell (either in positive or negative direction).
    For example, the PBC shift vector :math:`n = (1,0,-2)` would indicate that, in order to get from input point :math:`r_a` to input point :math:`r_b`, one has to cross the
    cell boundaries once in the positive x-direction and twice in the negative z-direction. If *return_pbcvec* is True,
    the method returns the tuple (:math:`{\Delta'}`, :math:`n`); otherwise it returns just :math:`{\Delta'}`.
    Note that the vector :math:`n` computed by this method can be used, for instance, to correctly initialize the :py:attr:`Bonds.pbc_vectors <ovito.data.Bonds.pbc_vectors>`
    property for newly created bonds that cross a periodic cell boundary.

    :param ra: The Cartesian xyz coordinates of the first input point(s). Either a 1-D array of length 3 or a 2-D array of shape (*N*,3).
    :param rb: The Cartesian xyz coordinates of the second input point(s). Must have the same shape as *ra*.
    :param bool return_pbcvec: If True, also returns the vector :math:`n`, which specifies how often the vector :math:`(r_b'r - r_a)` crosses the periodic cell boundaries.
    :returns: The vector :math:`{\Delta'}` and, optionally, the vector :math:`n`.

    Note that there exists also a convenience method :py:meth:`Particles.delta_vector() <ovito.data.Particles.delta_vector>`,
    which should be used in cases where :math:`r_a` and :math:`r_b` are both positions of particles in the simulation cell.
    """
    return self._delta_vector(rb - ra, return_pbcvec)
SimulationCell.delta_vector = _SimulationCell_delta_vector
