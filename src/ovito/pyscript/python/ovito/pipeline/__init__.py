"""
This module contains object types that are part of OVITO's data pipeline system.

**Pipelines:**

  * :py:class:`Pipeline`
  * :py:class:`Modifier` (base class)

**Data sources:**

  * :py:class:`StaticSource`
  * :py:class:`FileSource`

"""

# Load the native modules and other dependencies.
from ..plugins.PyScript import StaticSource, ModifierApplication, Modifier

# Load submodules.
from .pipeline_class import Pipeline
from .file_source import FileSource

__all__ = ['Pipeline', 'Modifier', 'StaticSource', 'FileSource', 'ModifierApplication']