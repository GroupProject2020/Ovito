from ovito.io import *
from ovito.data import *
from ovito.pipeline import StaticSource

pipeline = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')
data = pipeline.compute()

# Testing global attribute access:
print("Attributes:")
attribute_count = len(data.attributes)
assert(attribute_count > 0)
for attr in data.attributes:
    print(attr, data.attributes[attr])
data.attributes['new_attribute'] = 2.8
assert(len(data.attributes) == attribute_count + 1)
assert(data.attributes['new_attribute'] == 2.8)

# Testing find() and expect() methods:
cell = SimulationCell()
data = DataCollection()
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

old_particles = data.particles
new_particles = data.make_mutable(old_particles)
print(old_particles, new_particles)
assert(old_particles is not new_particles)
