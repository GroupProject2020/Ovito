# Load dependencies
import ovito
from ovito.data import DataCollection

# Load the native code module
from ovito.plugins.Particles import Particles

# Implementation of the DataCollection.particles attribute.
def _DataCollection_particles(self):
    """
    Returns the :py:class:`Particles` object, which stores the particles information. 
    """
    return self.find(Particles)
DataCollection.particles = property(_DataCollection_particles)

# Implementation of the DataCollection.particles_ attribute.
DataCollection.particles_ = property(lambda self: self.make_mutable(self.particles))

# Implementation of the DataCollection.bonds attribute.
# Here only for backward compatibility with OVITO 2.9.0.
DataCollection.bonds = property(lambda self: self.particles.bonds)

# Implementation of the DataCollection.bonds_ attribute.
DataCollection.bonds_ = property(lambda self: self.particles_.bonds_)

# Implementation of the DataCollection.number_of_particles property.
# Here only for backward compatibility with OVITO 2.9.0.
DataCollection.number_of_particles = property(lambda self: self.particles.count)

# Extend the DataCollection class with a 'number_of_half_bonds' property.
# Here only for backward compatibility with OVITO 2.9.0.
DataCollection.number_of_half_bonds = property(lambda self: self.number_of_full_bonds * 2)

# Extend the DataCollection class with a 'number_of_full_bonds' property.
# Here only for backward compatibility with OVITO 2.9.0.
DataCollection.number_of_full_bonds = property(lambda self: self.bonds.count)

# Extend the DataCollection class with a 'number_of_bonds' property.
# This is for backward compatibility with OVITO 2.8.2:
DataCollection.number_of_bonds = property(lambda self: self.number_of_half_bonds)

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
    return ase_to_ovito(atoms)
DataCollection.create_from_ase_atoms = classmethod(_DataCollection_create_from_ase_atoms)

# Implementation of the DataCollection.create_particle_property() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_particle_property(self, property_type, data = None):
    return self.particles_.create_property(property_type, data = data)
DataCollection.create_particle_property = _DataCollection_create_particle_property

# Implementation of the DataCollection.create_user_particle_property() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_user_particle_property(self, name, data_type, num_components=1, data = None):
    if data_type == 'int': data_type = int
    elif data_type == 'float': data_type = float
    return self.particles_.create_property(name, dtype = data_type, components = num_components, data = data)
DataCollection.create_user_particle_property = _DataCollection_create_user_particle_property

# Implementation of the DataCollection.create_bond_property() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_bond_property(self, property_type, data = None):
    return self.bonds_.create_property(property_type, data = data)
DataCollection.create_bond_property = _DataCollection_create_bond_property

# Implementation of the DataCollection.create_user_bond_property() method.
# Here only for backward compatibility with OVITO 2.9.0.
def _DataCollection_create_user_bond_property(self, name, data_type, num_components=1, data = None):
    if data_type == 'int': data_type = int
    elif data_type == 'float': data_type = float
    return self.bonds_.create_property(name, dtype = data_type, components = num_components, data = data)
DataCollection.create_user_bond_property = _DataCollection_create_user_bond_property

# Implementation of the DataCollection.particle_properties attribute.
# Here only for backward compatibility with OVITO 2.9.0.
DataCollection.particle_properties = property(lambda self: self.particles)

# Implementation of the DataCollection.bond_properties attribute.
# Here only for backward compatibility with OVITO 2.9.0.
DataCollection.bond_properties = property(lambda self: self.bonds)
