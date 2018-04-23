"""
This module contains object types that are part of OVITO's data pipeline system.

**Pipeline:**

  * :py:class:`Pipeline`
  * :py:class:`Modifier`

**Pipeline data sources:**

  * :py:class:`StaticSource`
  * :py:class:`FileSource`

"""

# Load the native modules and other dependencies.
from ..plugins.PyScript import StaticSource, Modifier, ModifierApplication

# Load submodules.
from .pipeline_class import Pipeline
from .file_source import FileSource

__all__ = ['Pipeline', 'Modifier', 'StaticSource', 'FileSource', 'ModifierApplication']