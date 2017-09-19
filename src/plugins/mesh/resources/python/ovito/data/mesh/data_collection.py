# Load dependencies
import ovito
from ovito.data import DataCollection

# Load the native code module
from ovito.plugins.Mesh import SurfaceMesh

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
