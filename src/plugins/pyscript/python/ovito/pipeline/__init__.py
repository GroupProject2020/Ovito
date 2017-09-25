"""
This module contains object types that are part of OVITO's data pipeline system.

**Pipeline:**

  * :py:class:`Pipeline`
  * :py:class:`Modifier`

**Pipeline data sources:**

  * :py:class:`StaticSource`
  * :py:class:`FileSource`
  * :py:class:`TrajectoryLineGenerator`

"""

# Load the native modules and other dependencies.
from ..plugins.PyScript import StaticSource, Modifier, ModifierApplication
from ..data import DataCollection

# Load submodules.
from .pipeline_class import Pipeline
from .file_source import FileSource

# Make StaticSource a DataCollection.
DataCollection.registerDataCollectionType(StaticSource)
assert(issubclass(StaticSource, DataCollection))

__all__ = ['Pipeline', 'Modifier', 'StaticSource', 'FileSource', 'ModifierApplication']