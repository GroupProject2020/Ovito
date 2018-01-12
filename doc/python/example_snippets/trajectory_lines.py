from ovito.pipeline import TrajectoryLineGenerator, Pipeline
from ovito.io import import_file
from ovito.vis import TrajectoryLineDisplay

# Load a particle simulation sequence:
pipeline = import_file('input/simulation.*.dump')

# Create a second pipeline for the trajectory lines:
traj_pipeline = Pipeline()

# Create data source and assign it to the second pipeline.
# The first pipeline provides the particle positions.
traj_pipeline.source = TrajectoryLineGenerator(
    source_pipeline = pipeline,
    only_selected = False
)

# Generate the trajectory lines by sampling the 
# particle positions over the entire animation interval.
traj_object = traj_pipeline.source.generate()

# Configure trajectory line display:
traj_object.display.width = 0.4
traj_object.display.color = (1,0,0)
traj_object.display.shading = TrajectoryLineDisplay.Shading.Flat

# Insert both pipelines into the scene to make both the particles and 
# the trajectory lines visible in rendered images:
pipeline.add_to_scene()
traj_pipeline.add_to_scene()
