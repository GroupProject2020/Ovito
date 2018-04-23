import ovito
from ovito.data import DataCollection, SimulationCell, ParticleType
from ovito.pipeline import StaticSource, Pipeline

# Create the data collection (initially empty):
data = DataCollection()

# XYZ coordinates of the three atoms to create:
pos = [(1.0, 1.5, 0.3),
       (7.0, 4.2, 6.0),
       (5.0, 9.2, 8.0)]

# Create the particle position property:
pos_prop = data.particles.create_property('Position', data=pos)

# Create the particle type property and insert two atom types:
type_prop = data.particles.create_property('Particle Type')
type_prop.types.append(ParticleType(id = 1, name = 'Cu', color = (0.0,1.0,0.0)))
type_prop.types.append(ParticleType(id = 2, name = 'Ni', color = (0.0,0.5,1.0)))
with type_prop:
    type_prop[0] = 1  # First atom is Cu
    type_prop[1] = 2  # Second atom is Ni
    type_prop[2] = 2  # Third atom is Ni

# Create a user-defined particle property with some data:
my_data = [3.141, -1.2, 0.23]
my_prop = data.particles.create_property('My property', data=my_data)

# Create the simulation box:
cell = SimulationCell(pbc = (False, False, False))
with cell:
    cell[...] = [[10,0,0,0],
                 [0,10,0,0],
                 [0,0,10,0]]
cell.vis.line_width = 0.1
data.objects.append(cell)

# Create 3 bonds between particles:
bond_topology = [[0,1], [1,2], [2,0]]
data.bonds.create_property('Topology', data=bond_topology)

# Create a pipeline, set source and insert it into the scene:
pipeline = Pipeline(source = StaticSource())
pipeline.source.assign(data)
pipeline.add_to_scene()