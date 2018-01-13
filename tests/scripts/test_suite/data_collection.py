from ovito.io import *
from ovito.data import *
from ovito.pipeline import StaticSource

pipeline = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')
assert(isinstance(pipeline.source, DataCollection))

print("Attributes:")
for attr in pipeline.source.attributes:
    print(attr, pipeline.source.attributes[attr])

dc = StaticSource()
assert(isinstance(dc, DataCollection))
dc.attributes['new_attribute'] = 2.8
cell = SimulationCell()
dc.objects.append(cell)
assert(dc.find(SimulationCell) is cell)
assert(dc.expect(SimulationCell) is cell)
assert(dc.find_all(SimulationCell) == [cell])
dc.objects.remove(cell)
assert(dc.find(SimulationCell) is None)

print()
print("Particle properties:")
for p in pipeline.source.particle_properties:
    print(p)

print()
print("Particles:")
print(pipeline.source.number_of_particles)

print()
print("Bond properties:")
for p in pipeline.source.bond_properties:
    print(p)

print()
print("Cell:")
print(pipeline.source.cell)

print()
print("Bonds:")
print(pipeline.source.number_of_full_bonds)
print(pipeline.source.number_of_half_bonds)
print(pipeline.source.bonds)

old_prop = pipeline.source.particle_properties.molecule_identifier
new_prop = pipeline.source.copy_if_needed(old_prop)
print(old_prop, new_prop)
assert(old_prop is new_prop)

data = pipeline.compute()

old_prop = data.particle_properties.molecule_identifier
new_prop = data.copy_if_needed(old_prop)
print(old_prop, new_prop)
assert(old_prop is not new_prop)

mass_prop = pipeline.source.create_particle_property(ParticleProperty.Type.Mass)
mass_prop.marray[0] = 0.5

color_prop = pipeline.source.create_bond_property(BondProperty.Type.Color)
color_prop.marray[0] = (0.5, 0.1, 0.9)
