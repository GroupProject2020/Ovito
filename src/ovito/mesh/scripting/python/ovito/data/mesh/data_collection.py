# Load dependencies
import ovito
from ovito.data import DataCollection
from ovito.data.data_objects_dict import DataObjectsDict

# Load the native code module.
from ovito.plugins.MeshPython import SurfaceMesh

# Implementation of the DataCollection.surfaces attribute.
def _DataCollection_surfaces(self):
    """
    Returns a dictionary view providing key-based access to all :py:class:`SurfaceMesh` objects in 
    this data collection. Each :py:class:`SurfaceMesh` has a unique :py:attr:`~ovito.data.DataObject.identifier` key, 
    which can be used to look it up in the dictionary. 
    See the documentation of the modifier producing the surface mesh to find out what the right key is, or use

    .. literalinclude:: ../example_snippets/data_collection_surfaces.py
        :lines: 9-9

    to see which identifier keys exist. Then retrieve the desired :py:class:`SurfaceMesh` object from the collection using its identifier 
    key, e.g.

    .. literalinclude:: ../example_snippets/data_collection_surfaces.py
        :lines: 14-15
    """
    return DataObjectsDict(self, SurfaceMesh)
DataCollection.surfaces = property(_DataCollection_surfaces)

# Implementation of the DataCollection.surface attribute.
# This attribute has been deprecated and is here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_surface(self):
    # Returns the :py:class:`SurfaceMesh` in this :py:class:`!DataCollection`.    
    # Accessing this property raises an ``AttributeError`` if the data collection
    # contains no surface mesh instance.
    for obj in self.objects:
        if isinstance(obj, SurfaceMesh):
            return obj
    raise AttributeError("This DataCollection contains no surface mesh.")
DataCollection.surface = property(_DataCollection_surface)
