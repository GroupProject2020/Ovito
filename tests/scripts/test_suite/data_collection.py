from ovito.io import *
from ovito.data import *
from ovito.pipeline import StaticSource

pipeline = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')
data = pipeline.compute()

# Testing gloabl attribute access:
print("Attributes:")
for attr in data.attributes:
    print(attr, data.attributes[attr])
data = DataCollection()
data.attributes['new_attribute'] = 2.8

# Testing find() and expect() methods:
cell = SimulationCell()
data.objects.append(cell)
assert(data.find(SimulationCell) is cell)
assert(data.expect(SimulationCell) is cell)
assert(data.find_all(SimulationCell) == [cell])
assert(data.copy_if_needed(cell) is cell)
data.objects.remove(cell)
assert(data.find(SimulationCell) is None)

data = pipeline.compute()

print()
print("Particle properties:")
for p in data.particles:
    print(p)

print()
print("Particles:")
print(data.number_of_particles)

print()
print("Bond properties:")
for p in data.bonds:
    print(p)

print()
print("Cell:")
print(data.cell)

print()
print("Bonds:")
print(data.number_of_full_bonds)
print(data.number_of_half_bonds)
print(data.bonds)

old_prop = data.particles['Molecule Identifier']
new_prop = data.copy_if_needed(old_prop)
print(old_prop, new_prop)
assert(old_prop is not new_prop)

# Testing backward compatibility with OVITO 2.9.0:
mass_prop = data.create_particle_property(ParticleProperty.Type.Mass)
mass_prop.marray[0] = 0.5
assert(mass_prop in data.particles.values())
color_prop = data.create_bond_property(BondProperty.Type.Color)
color_prop.marray[0] = (0.5, 0.1, 0.9)
assert(color_prop in data.bonds.values())
