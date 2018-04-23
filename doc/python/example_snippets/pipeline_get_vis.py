from ovito.io import import_file
from ovito.vis import ParticlesVis

pipeline = import_file('input/simulation.dump')

# >>>>>>>>>>>>>>>>> snippet begin >>>>>>>>>>>>>>>>
from ovito.vis import ParticlesVis

# Access the visual element responsible for the particle display:
pipeline.get_vis(ParticlesVis).radius = 1.3

# Note that get_vis() has returned the ParticlesVis element that is attached 
# to the 'Position' particle property:
assert(pipeline.compute().particles['Position'].vis.radius == 1.3)
# <<<<<<<<<<<<<<<<< snippet end <<<<<<<<<<<<<<<<<
