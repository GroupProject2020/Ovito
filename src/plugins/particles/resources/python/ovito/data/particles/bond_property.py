# Load dependencies
import ovito
import ovito.data

# Load the native code module
from ovito.plugins.Particles import BondProperty

def _BondProperty_type_by_id(self, id):
    """
    Returns the :py:class:`BondType` with the given numeric ID from the :py:attr:`.types` list. 
    Raises a ``KeyError`` if the ID does not exist.
    """
    t = self._get_type_by_id(int(id))
    if t is None:
        raise KeyError("Bond type with ID %i is not defined." % id)
    return t
BondProperty.type_by_id = _BondProperty_type_by_id

def _BondProperty_type_by_name(self, name):
    """
    Returns the :py:class:`BondType` with the given name from the :py:attr:`.types` list.
    If multiple types exists with the same name, the first type is returned. 
    Raises a ``KeyError`` if there is no type with such a name.
    """
    t = self._get_type_by_name(name)
    if t is None:
        raise KeyError("Bond type with name '%s' is not defined." % name)
    return t
BondProperty.type_by_name = _BondProperty_type_by_name

# For backward compatibility with OVITO 2.9.0:
BondProperty.get_type_by_id = lambda self, id: self.type_by_id(id)
BondProperty.get_type_by_name = lambda self, name: self.type_by_name(name)
BondProperty.type_list = property(lambda self: self.types)
