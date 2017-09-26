# Load dependencies
import ovito

# Load the native code module
from ovito.plugins.StdObj import SimulationCell

# Implement indexing for simulation cell object.
SimulationCell.__getitem__ = lambda self, idx: self.matrix[idx]

# Implement iteration for simulation cell object.
SimulationCell.__iter__ = lambda self: iter(self.matrix)

# Implement SimulationCell.ndim attribute.
SimulationCell.ndim = property(lambda self: 2)

# Implement SimulationCell.shape attribute.
SimulationCell.shape = property(lambda self: (3, 4))

# Implement SimulationCell.__array_interface__ attribute.
SimulationCell.__array_interface__ = property(lambda self: self.matrix.__array_interface__)

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

# Implementation of SimulationCell.modify() method:
def _SimulationCell_modify(self):

    # A Python context manager for managing write access to the internal cell vector data.
    # It returns a writable Numpy array when used in a Python 'with' statement.
    class _CellModificationManager:

        def __init__(self, owner):
            """ Constructor that stores away a back-pointer to the owning object. """
            self._owner = owner

        def __enter__(self):
            """ Enter the runtime context related to this object. """
            # Get a modifiable Numpy array from the SimulationCell object and return it.
            self._matrix = self._owner.matrix.copy()
            return self._matrix

        def __exit__(self, exc_type, exc_value, traceback):
            """ Exit the runtime context related to this object. """
            # Write changed cell matrix back to the SimulationCell object.
            if exc_type is None:
               self._owner.matrix = self._matrix
            return False

    return _CellModificationManager(self)

SimulationCell.modify = _SimulationCell_modify