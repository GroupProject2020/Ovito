"""
This module contains object types that represent various kinds of data, which are produced and processed by OVITO's data pipeline system.
It also provides container classes for such data objects and some additional utility classes to work with neighbor lists and bonds.

**Data collection types:**

  * :py:class:`DataCollection`
  * :py:class:`PipelineFlowState`
  
**Data objects:**

  * :py:class:`DataObject` (base of all data object types)
  * :py:class:`Bonds`
  * :py:class:`BondProperty`
  * :py:class:`DislocationNetwork`
  * :py:class:`ParticleProperty`
  * :py:class:`Property`
  * :py:class:`SimulationCell`
  * :py:class:`SurfaceMesh`

**Auxiliary data classes:**

  * :py:class:`ParticleType`
  * :py:class:`BondType`
  * :py:class:`DislocationSegment`
  * :py:class:`ParticlePropertiesView`
  * :py:class:`BondPropertiesView`

**Utility classes:**

  * :py:class:`CutoffNeighborFinder`
  * :py:class:`NearestNeighborFinder`
  * :py:class:`BondsEnumerator`

"""

import numpy as np

# Load the native modules.
from ..plugins.PyScript import PipelineFlowState, DataObject, CloneHelper

# Load submodules.
from .data_collection import DataCollection
from .simulation_cell import SimulationCell
from .property import Property

# Make PipelineFlowState a DataCollection.
DataCollection.registerDataCollectionType(PipelineFlowState)
assert(issubclass(PipelineFlowState, DataCollection))

__all__ = ['DataCollection', 'DataObject', 'PipelineFlowState', 'SimulationCell', 'Property']