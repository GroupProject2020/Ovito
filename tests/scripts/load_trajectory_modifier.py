from ovito.io import import_file
from ovito.modifiers import LoadTrajectoryModifier
import ovito
import numpy

# Load static topology data from a LAMMPS data file.
pipeline = import_file("../files/LAMMPS/water.start.data.gz")
pipeline.add_to_scene()

# Load atom trajectories from separate LAMMPS dump file.
traj_mod = LoadTrajectoryModifier()
traj_mod.source.load("../files/LAMMPS/water.unwrapped.lammpstrj.gz")

# Insert modifier into data pipeline.
pipeline.modifiers.append(traj_mod)

print("Number of source frames:", pipeline.source.num_frames)
print("Number of trajectory frames:", traj_mod.source.num_frames)
print("Number of animation frames:", ovito.scene.anim.last_frame - ovito.scene.anim.first_frame + 1)
assert(traj_mod.source.num_frames == 2)
assert(ovito.scene.anim.last_frame == 1)

for frame in range(ovito.scene.anim.first_frame, ovito.scene.anim.last_frame + 1):
    data = pipeline.compute(frame)
    print("Frame %i:" % frame)
    print(data.particles['Position'][...])

pos0 = pipeline.compute(0).particles['Position']
pos1 = pipeline.compute(1).particles['Position']

assert(not numpy.allclose(pos0, pos1))
