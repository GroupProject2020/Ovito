"""
This module contains object types that represent various kinds of data, which are produced and processed by OVITO's data pipeline system.
It also provides container classes for such data objects and some additional utility classes to work with neighbor lists and bonds.

**Data collection types:**

  * :py:class:`DataCollection`
  * :py:class:`PipelineFlowState`
  
**Data objects:**

  * :py:class:`DataObject` (base of all data object types)
  * :py:class:`Property`
  * :py:class:`ParticleProperty`
  * :py:class:`BondProperty`
  * :py:class:`SimulationCell`
  * :py:class:`SurfaceMesh`
  * :py:class:`DislocationNetwork`

**Auxiliary data classes:**

  * :py:class:`ParticlesView`
  * :py:class:`ParticleType`
  * :py:class:`BondsView`
  * :py:class:`BondType`
  * :py:class:`DislocationSegment`

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

# Make PipelineFlowState a DataCollection.
DataCollection.registerDataCollectionType(PipelineFlowState)
assert(issubclass(PipelineFlowState, DataCollection))

__all__ = ['DataCollection', 'DataObject', 'PipelineFlowState']
