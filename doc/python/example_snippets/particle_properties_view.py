from ovito.io import import_file
from ovito.data import ParticleProperty
import ovito.pipeline
import numpy
pipeline = import_file("input/simulation.dump")
# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data = pipeline.compute()

positions = data.particle_properties['Position']
has_selection = 'Selection' in data.particle_properties
name_list = data.particle_properties.keys()
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
num_particles = len(data.particle_properties['Position'])
colors = numpy.random.random_sample(size = (num_particles, 3))
data.particle_properties.create('Color', data=colors)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
prop = data.particle_properties.create('Color')
with prop:
    prop[...] = numpy.random.random_sample(size = prop.shape)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
values = numpy.arange(0, num_particles, dtype=int)
data.particle_properties.create('myint', data=values)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
prop = data.particle_properties.create('myvector', dtype=float, components=3)
with prop:
    prop[...] = numpy.random.random_sample(size = prop.shape)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# An empty data collection to begin with:
data = ovito.pipeline.StaticSource()

# Create 10 particles with random xyz coordinates:
positions = numpy.random.random_sample(size = (10,3))
data.particle_properties.create('Position', data=positions)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
