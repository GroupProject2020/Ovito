from ovito.io import import_file
from ovito.vis import ParticlesVis

pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()

vis_element = pipeline.source.data.particles.vis
vis_element.shape = ParticlesVis.Shape.Square