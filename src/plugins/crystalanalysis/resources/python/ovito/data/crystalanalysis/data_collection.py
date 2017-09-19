# Load dependencies
import ovito
from ovito.data import DataCollection

# Load the native code module
from ovito.plugins.CrystalAnalysis import DislocationNetwork

# Implementation of the DataCollection.dislocations attribute.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_dislocations(self):
    # Returns the :py:class:`DislocationNetwork` object in this :py:class:`!DataCollection`.
    return self.find(DislocationNetwork)
DataCollection.dislocations = property(_DataCollection_dislocations)
