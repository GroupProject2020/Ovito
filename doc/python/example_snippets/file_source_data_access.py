from ovito.io import import_file

############## Code snippet begins here ##################
# This creates a new Pipeline with an attached FileSource.
pipeline = import_file('input/simulation.dump')

# Request data of animation frame 0 from the FileSource.
data = pipeline.source.compute(0)
print(data.particles.positions[...])
############## Code snippet ends here ##################


############## Code snippet begins here ##################
# The data cache of the FileSource:
data = pipeline.source.data

# Specify atom type names and radii:
type_list = data.particles.particle_types.types
type_list[0].name = "Cu"
type_list[0].radius = 1.35
type_list[1].name = "Zr"
type_list[1].radius = 1.55
############## Code snippet ends here ##################
