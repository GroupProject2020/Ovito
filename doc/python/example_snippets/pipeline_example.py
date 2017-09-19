from ovito.io import import_file
from ovito.modifiers import SliceModifier

# Import a simulation file. This creates a Pipeline object.
pipeline = import_file('simulation.dump')

# Insert a modifier that operates on the data:
pipeline.modifiers.append(SliceModifier(normal=(0,0,1), distance=0))

# Compute the effect of the slice modifier by evaluating the pipeline.
output = pipeline.compute()
print("Remaining particle count:", len(output.particle_properties['Position']))

# Access the cached input data of the FileSource:
input = pipeline.source
print("Input particle count:", len(input.particle_properties['Position']))
