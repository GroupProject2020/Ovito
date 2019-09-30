

#----------------- Begin of snippet -----------------------
from ovito.io import import_file
from ovito.pipeline import FileSource

# Load the reference particle configuration in a one-time initialization step
# outside of the modifier function:
reference = FileSource()
reference.load("input/simulation.0.dump", sort_particles=True)

def modify(frame, data):
    displacements = data.particles.positions - reference.data.particles.positions
    data.particles_.create_property('Displacement', data=displacements)
#----------------- End of snippet -----------------------

# Private code for testing the mdoifier function:
from ovito.io import export_file
pipeline = import_file("input/simulation.*.dump", sort_particles=True)
pipeline.modifiers.append(modify)
export_file(pipeline, "output/displacements.xyz", "xyz", columns=['Displacement.X', 'Displacement.Y', 'Displacement.Z'], multiple_frames=True)