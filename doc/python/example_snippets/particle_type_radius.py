from ovito.io import import_file

####### Snippet begins here ###########
pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()

types = pipeline.source.data.particles.particle_types
types.type_by_id(1).name = "Cu"
types.type_by_id(1).radius = 1.35
types.type_by_id(2).name = "Zr"
types.type_by_id(2).radius = 1.55