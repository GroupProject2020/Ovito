from ovito.io import import_file
from ovito.modifiers import GenerateTrajectoryLinesModifier
from ovito.vis import TrajectoryVis

# Load a particle simulation sequence:
pipeline = import_file('input/simulation.*.dump')

# Insert the modifier into the pipeline for creating the trajectory lines.
modifier = GenerateTrajectoryLinesModifier(only_selected = False)
pipeline.modifiers.append(modifier)

# Now let the modifier generate the trajectory lines by sampling the 
# particle positions over the entire animation interval.
modifier.generate()

# Configure trajectory line visualization:
modifier.vis.width = 0.4
modifier.vis.color = (1,0,0)
modifier.vis.shading = TrajectoryVis.Shading.Flat

# Insert pipeline into the scene to make the particles and 
# the trajectory lines visible in rendered images.
pipeline.add_to_scene()
