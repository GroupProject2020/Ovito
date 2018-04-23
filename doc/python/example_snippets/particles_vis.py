from ovito.io import import_file
from ovito.vis import ParticlesVis

pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()
vis = pipeline.get_vis(ParticlesVis)
vis.shape = ParticlesVis.Shape.Square