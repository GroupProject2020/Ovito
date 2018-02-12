from ovito.io import import_file
from ovito.vis import ParticleVis

pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()
vis = pipeline.source.particle_properties.position.vis
vis.shape = ParticleVis.Shape.Square