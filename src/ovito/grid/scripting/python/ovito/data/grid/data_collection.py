# Load dependencies
import ovito
from ovito.data import DataCollection
from ovito.data.data_objects_dict import DataObjectsDict

# Load the native code module
from ovito.plugins.GridPython import VoxelGrid

# Implementation of the DataCollection.grids attribute.
def _DataCollection_grids(self):
    """
    Returns a dictionary view providing key-based access to all :py:class:`VoxelGrids <VoxelGrid>` in 
    this data collection. Each :py:class:`VoxelGrid` has a unique :py:attr:`~ovito.data.DataObject.identifier` key, 
    which you can use to look it up in this dictionary. To find out which voxel grids exist in the data collection and what 
    their identifier keys are, use

    .. literalinclude:: ../example_snippets/data_collection_grids.py
        :lines: 7-7

    Then retrieve the desired :py:class:`VoxelGrid` from the collection using its identifier key, e.g.

    .. literalinclude:: ../example_snippets/data_collection_grids.py
        :lines: 12-13
    """
    return DataObjectsDict(self, VoxelGrid)
DataCollection.grids = property(_DataCollection_grids)