import numpy

# Load dependencies
import ovito
import ovito.data

# Load the native code module
from ovito.plugins.Particles import Bonds

# Implement indexing for bond arrays.
Bonds.__getitem__ = lambda self, idx: numpy.asarray(self)[idx]

# Implement iteration for bond arrays.
Bonds.__iter__ = lambda self: iter(numpy.asarray(self))

# Implement Bonds.ndim attribute.
Bonds.ndim = property(lambda self: 2)

# Implement Bonds.shape attribute.
Bonds.shape = property(lambda self: (len(self), 2))

# Returns a NumPy array wrapper for the bond PBC shift vectors.
def _Bonds_pbc_vectors(self):
    """ A NumPy array providing read access to the PBC shift vectors of bonds.
        
        The returned array's shape is *N x 3*, where *N* is the number of bonds. It contains the
        periodic shift vector for each bond.
        
        A PBC shift vector consists of three integers, which specify how many times (and in which direction)
        the corresonding bond crosses the periodic boundaries of the simulation cell when going from the first particle 
        to the second. For example, a shift vector (0,-1,0)
        indicates that the bond crosses the periodic boundary in the negative Y direction 
        once. In other words, the particle where the bond originates from is located
        close to the lower edge of the simulation cell in the Y direction while the second particle is located 
        close to the opposite side of the box.
        
        The PBC shift vectors are important for visualizing the bonds between particles with wrapped coordinates, 
        which are located on opposite sides of a periodic cell. When the PBC shift vector of a bond is (0,0,0), OVITO assumes that 
        both particles connected by the bond are part of the same periodic image and the bond is rendered such that
        it directly connects the two particles without going through a cell boundary.
    """
    class DummyClass:
        pass
    o = DummyClass()
    o.__array_interface__ = self._pbc_vectors
    # Create reference to particle property object to keep it alive.
    o.__base_property = self
    return numpy.asarray(o)
Bonds.pbc_vectors = property(_Bonds_pbc_vectors)

# Returns a NumPy array wrapper for bonds list.
# This has been deprecated. Here only for backward compatibility with OVITO 2.9.0.
def _Bonds_array(self):
    # This attribute returns a NumPy array providing direct access to the bonds list.
    return numpy.asarray(self)
Bonds.array = property(_Bonds_array)

# Register BondsEnumerator as a 'pseudo' nested class of Bonds.
# This is for backward compatibility with OVITO 2.9.0.
from ovito.plugins.Particles import BondsEnumerator
Bonds.Enumerator = lambda self: BondsEnumerator(self)
