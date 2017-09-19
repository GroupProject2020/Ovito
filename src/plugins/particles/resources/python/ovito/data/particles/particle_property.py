# Load dependencies
import ovito
import ovito.data

# Load the native code module
from ovito.plugins.Particles import ParticleProperty

def _ParticleProperty_type_by_id(self, id):
    """
    Looks up the :py:class:`ParticleType` with the given numeric ID in the :py:attr:`.types` list. 
    Raises a ``KeyError`` if the ID does not exist.
    """
    t = self._get_type_by_id(int(id))
    if t is None:
        raise KeyError("Particle type with ID %i is not defined." % id)
    return t
ParticleProperty.type_by_id = _ParticleProperty_type_by_id

def _ParticleProperty_type_by_name(self, name):
    """
    Looks up the :py:class:`ParticleType` with the given name in the :py:attr:`.types` list.
    If multiple types exists with the same name, the first type is returned. 
    Raises a ``KeyError`` if there is no type with such a name.
    """
    t = self._get_type_by_name(name)
    if t is None:
        raise KeyError("Particle type with name '%s' is not defined." % name)
    return t
ParticleProperty.type_by_name = _ParticleProperty_type_by_name

# For backward compatibility with OVITO 2.9.0:
ParticleProperty.get_type_by_id = lambda self, id: self.type_by_id(id)
ParticleProperty.get_type_by_name = lambda self, name: self.type_by_name(name)
ParticleProperty.type_list = property(lambda self: self.types)