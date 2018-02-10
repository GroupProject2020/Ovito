from ovito.io import import_file
from ovito.modifiers import LoadTrajectoryModifier

# Load static topology data from a LAMMPS data file.
pipeline = import_file('input/input.data', atom_style='bond')

# Load atom trajectories from separate LAMMPS dump file.
traj_mod = LoadTrajectoryModifier()
traj_mod.source.load('input/trajectory.dump')
print("Number of frames: ", traj_mod.source.num_frames)

# Insert modifier into data pipeline.
pipeline.modifiers.append(traj_mod)