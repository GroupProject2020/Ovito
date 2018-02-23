from ovito.io import import_file
from ovito.data import ParticleProperty
import ovito.pipeline
import numpy
pipeline = import_file("input/simulation.dump")
# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data = pipeline.compute()

positions = data.particles['Position']
has_selection = 'Selection' in data.particles
name_list = data.particles.keys()
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
colors = numpy.random.random_sample(size = (data.particles.count, 3))
data.particles.create_property('Color', data=colors)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
prop = data.particles.create_property('Color')
with prop:
    prop[...] = numpy.random.random_sample(size = prop.shape)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
values = numpy.arange(0, data.particles.count, dtype=int)
data.particles.create_property('myint', data=values)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
prop = data.particles.create_property('myvector', dtype=float, components=3)
with prop:
    prop[...] = numpy.random.random_sample(size = prop.shape)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# An empty data collection to begin with:
data = ovito.pipeline.StaticSource()

# Create 10 particles with random xyz coordinates:
positions = numpy.random.random_sample(size = (10,3))
data.particles.create_property('Position', data=positions)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
