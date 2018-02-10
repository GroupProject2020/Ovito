from ovito.io import import_file
from ovito.modifiers import LoadTrajectoryModifier
import ovito
import numpy

# Load static topology data from a LAMMPS data file.
pipeline = import_file("../../files/LAMMPS/water.start.data.gz")

# Load atom trajectories from separate LAMMPS dump file.
traj_mod = LoadTrajectoryModifier()
traj_mod.source.load("../../files/LAMMPS/water.unwrapped.lammpstrj.gz")

# Insert modifier into data pipeline.
pipeline.modifiers.append(traj_mod)

print("Number of source frames:", pipeline.source.num_frames)
print("Number of trajectory frames:", traj_mod.source.num_frames)
print("Number of animation frames:", ovito.dataset.anim.last_frame - ovito.dataset.anim.first_frame + 1)
assert(traj_mod.source.num_frames == 2)
assert(ovito.dataset.anim.last_frame == 1)

for frame in range(ovito.dataset.anim.first_frame, ovito.dataset.anim.last_frame + 1):
    data = pipeline.compute(frame)
    print("Frame %i:" % frame)
    print(data.particle_properties['Position'][...])

pos0 = pipeline.compute(0).particle_properties['Position']
pos1 = pipeline.compute(1).particle_properties['Position']

assert(not numpy.allclose(pos0, pos1))
