"""
This module contains various types of data objects, which are produced and processed within OVITO's data pipeline system.
It also provides the :py:class:`DataCollection` container class for such data objects and some additional utility classes to
compute neighbor lists to iterate over the bonds of particles.

**Data container:**

  * :py:class:`DataCollection`
  
**Data objects:**

  * :py:class:`DataObject` (base of all data object types)
  * :py:class:`Property`
  * :py:class:`ParticleProperty`
  * :py:class:`BondProperty`
  * :py:class:`SimulationCell`
  * :py:class:`SurfaceMesh`
  * :py:class:`TrajectoryLines`
  * :py:class:`DislocationNetwork`
  * :py:class:`PlotData`

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
from ..plugins.PyScript import DataObject

# Load submodules.
from .data_collection import DataCollection

__all__ = ['DataCollection', 'DataObject']
