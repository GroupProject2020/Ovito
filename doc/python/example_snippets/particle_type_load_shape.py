from ovito.io import import_file

####### Snippet begins here ###########
pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()

types = pipeline.source.data.particles.particle_types
types.type_by_id(1).load_shape("input/tetrahedron.vtk")
types.type_by_id(1).highlight_edges = True