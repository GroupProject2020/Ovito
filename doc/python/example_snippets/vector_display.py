from ovito.io import import_file
from ovito.vis import VectorVis

node = import_file("input/simulation.dump")
node.add_to_scene()
vis = node.source.particle_properties.force.vis
vis.color = (1.0, 0.5, 0.5)