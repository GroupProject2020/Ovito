# Load dependencies
import ovito
from ovito.data import DataCollection
from ovito.data.data_objects_dict import DataObjectsDict

# Load the native code module
from ovito.plugins.Grid import VoxelGrid

# Implementation of the DataCollection.grids attribute.
def _DataCollection_grids(self):
    """
    Returns a :py:class:`DataObjectsDict` providing key-based access to all :py:class:`VoxelGrid` objects stored 
    in this :py:class:`!DataCollection`.
    """
    return DataObjectsDict(self, VoxelGrid)
DataCollection.grids = property(_DataCollection_grids)