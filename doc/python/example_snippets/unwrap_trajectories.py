from ovito.io import import_file
from ovito.modifiers import UnwrapTrajectoriesModifier

# Load a simulation trajectory:
pipeline = import_file('input/simulation.*.dump')

# Insert the unwrap modifier into the pipeline.
modifier = UnwrapTrajectoriesModifier()
pipeline.modifiers.append(modifier)
# Let the modifier determine crossings of the periodic cell boundaries.
modifier.update()

# Request last frame of the trajectory with unwrapped particle positions: 
data = pipeline.compute(pipeline.source.num_frames - 1)