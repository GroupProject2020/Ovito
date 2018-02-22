# Load dependencies
import ovito
from ovito.data import DataCollection
from .particles_view import ParticlesView
from .bonds_view import BondsView

# Load the native code module
from ovito.plugins.Particles import ParticleProperty, BondProperty

# Implementation of the DataCollection.particles attribute.
def _DataCollection_particles(self):
    """
    Returns a :py:class:`ParticlesView` object representing the particles, which provides name-based access to the :py:class:`ParticleProperty` 
    instances stored in this :py:class:`!DataCollection`. Furthermore, the view object provides convenience functions for creating new particle properties.
    """
    return ParticlesView(self)
DataCollection.particles = property(_DataCollection_particles)

# Implementation of the DataCollection.bonds attribute.
def _DataCollection_bonds(self):
    """
    Returns a :py:class:`BondsView`, which allows to access all :py:class:`BondProperty` 
    objects stored in this data collection by name. Furthermore, it provides convenience functions
    for adding new bond properties to the collection.
    """
    return BondsView(self)
DataCollection.bonds = property(_DataCollection_bonds)

# Implementation of the DataCollection.number_of_particles property.
# Here only for backward compatibility with OVITO 2.9.0.
def _get_DataCollection_number_of_particles(self):
    # The number of particles in the data collection. 
    # Implementation note: This value is derived from the length of the ``Position`` standard :py:class:`ParticleProperty` stored 
    # in this data collection. The number of particles is 0 if the ``Position`` particle property does not exist. """
    # The number of particles is determined by the size of the 'Position' particle property.
    for obj in self.objects:
        if isinstance(obj, ParticleProperty) and obj.type == ParticleProperty.Type.Position:
            return obj.size
    return 0
DataCollection.number_of_particles = property(_get_DataCollection_number_of_particles)

# Extend the DataCollection class with a 'number_of_half_bonds' property.
# Here only for backward compatibility with OVITO 2.9.0.
def _get_DataCollection_number_of_half_bonds(self):
    return self.number_of_full_bonds * 2
DataCollection.number_of_half_bonds = property(_get_DataCollection_number_of_half_bonds)

# Extend the DataCollection class with a 'number_of_full_bonds' property.
# Here only for backward compatibility with OVITO 2.9.0.
def _get_DataCollection_number_of_full_bonds(self):
    # The number of (full) bonds stored in the data collection. 
    for obj in self.objects:
        if isinstance(obj, Bonds):
            return obj.size
    return 0
DataCollection.number_of_full_bonds = property(_get_DataCollection_number_of_full_bonds)

# Implementation of the DataCollection.to_ase_atoms() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_to_ase_atoms(self):
    from ovito.io.ase import ovito_to_ase
    return ovito_to_ase(self)
DataCollection.to_ase_atoms = _DataCollection_to_ase_atoms

# Implementation of the DataCollection.create_from_ase_atoms() function.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_from_ase_atoms(cls, atoms):
    from ovito.io.ase import ase_to_ovito
    from ovito.pipeline import StaticSource
    data = StaticSource()
    ase_to_ovito(atoms, data)
    return data
DataCollection.create_from_ase_atoms = classmethod(_DataCollection_create_from_ase_atoms)

# Implementation of the DataCollection.create_particle_property() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_particle_property(self, property_type, data = None):
    return self.particles.create(property_type, data = data)
DataCollection.create_particle_property = _DataCollection_create_particle_property

# Implementation of the DataCollection.create_user_particle_property() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_user_particle_property(self, name, data_type, num_components=1, data = None):
    if data_type == 'int': data_type = int
    elif data_type == 'float': data_type = float
    return self.particles.create(name, dtype = data_type, components = num_components, data = data)
DataCollection.create_user_particle_property = _DataCollection_create_user_particle_property

# Implementation of the DataCollection.create_bond_property() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_bond_property(self, property_type, data = None):
    return self.bonds.create(property_type, data = data)
DataCollection.create_bond_property = _DataCollection_create_bond_property

# Implementation of the DataCollection.create_user_bond_property() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_user_bond_property(self, name, data_type, num_components=1, data = None):
    if data_type == 'int': data_type = int
    elif data_type == 'float': data_type = float
    return self.bonds.create(name, dtype = data_type, components = num_components, data = data)
DataCollection.create_user_bond_property = _DataCollection_create_user_bond_property

# Extend the DataCollection class with a 'number_of_bonds' property.
# This is for backward compatibility with OVITO 2.8.2:
def _get_DataCollection_number_of_bonds(self):
    return self.number_of_half_bonds
DataCollection.number_of_bonds = property(_get_DataCollection_number_of_bonds)

# Implementation of the DataCollection.particle_properties attribute.
# Here only for backward compatibility with OVITO 2.9.0.
DataCollection.particle_properties = property(lambda self: self.particles)

# Implementation of the DataCollection.bond_properties attribute.
# Here only for backward compatibility with OVITO 2.9.0.
DataCollection.bond_properties = property(lambda self: self.bonds)
