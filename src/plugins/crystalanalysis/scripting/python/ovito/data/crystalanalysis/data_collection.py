# Load dependencies
import ovito
from ovito.data import DataCollection

# Load the native code module
from ovito.plugins.CrystalAnalysisPython import DislocationNetwork

# Implementation of the DataCollection.dislocations attribute.
def _DataCollection_dislocations(self):
    """
    Returns the :py:class:`DislocationNetwork` data object from this data collection or ``None`` if there isn't one.
    """
    return self.find(DislocationNetwork)
DataCollection.dislocations = property(_DataCollection_dislocations)
