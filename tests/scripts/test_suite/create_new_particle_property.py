import ovito
from ovito.data import ParticleProperty, SimulationCell, Bonds, ParticleType
from ovito.pipeline import StaticSource, Pipeline

# The data collection, initially empty:
data = StaticSource()

# XYZ coordinates of the three atoms to create:
pos = [(1.0, 1.5, 0.3),
       (7.0, 4.2, 6.0),
       (5.0, 9.2, 8.0)]

# Create the particle position property:
pos_prop = data.particle_properties.create(ParticleProperty.Type.Position, data=pos)

# Create the particle type property and insert two atom types:
type_prop = data.particle_properties.create(ParticleProperty.Type.ParticleType)
type_prop.type_list.append(ParticleType(id = 1, name = 'Cu', color = (0.0,1.0,0.0)))
type_prop.type_list.append(ParticleType(id = 2, name = 'Ni', color = (0.0,0.5,1.0)))
with type_prop:
    type_prop[0] = 1  # First atom is Cu
    type_prop[1] = 2  # Second atom is Ni
    type_prop[2] = 2  # Third atom is Ni

# Create a user-defined particle property with some data:
my_data = [3.141, -1.2, 0.23]
my_prop = data.particle_properties.create('My property', data=my_data)

# Create the simulation box:
cell = SimulationCell(pbc = (True, True, True))
with cell:
    cell[...] = [[10,0,0,0],
                 [0,10,0,0],
                 [0,0,10,0]]
cell.display.line_width = 0.1
data.objects.append(cell)

# Create bonds between particles:
bonds = Bonds()
data.objects.append(bonds)
# TODO: Creation of new bonds is not implemented yet

# Create a pipeline and insert it into the scene:
pipeline = Pipeline(source = data)
pipeline.add_to_scene()

# Select the new pipeline and adjust cameras of all viewports to show it:
ovito.dataset.selected_pipeline = pipeline
for vp in ovito.dataset.viewports:
    vp.zoom_all()
