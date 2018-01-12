from ovito.io import import_file
from ovito.vis import ParticleDisplay

pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()
particle_display = pipeline.source.particle_properties.position.display
particle_display.shape = ParticleDisplay.Shape.Square