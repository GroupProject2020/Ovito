from ovito.io import import_file
from ovito.modifiers import LoadTrajectoryModifier

# Load static topology data from a LAMMPS data file.
pipeline = import_file('input.data', atom_style = 'bond')

# Load atom trajectories from separate LAMMPS dump file.
traj_mod = LoadTrajectoryModifier()
traj_mod.source.load('trajectory.dump', multiple_frames = True)

# Insert modifier into data pipeline.
pipeline.modifiers.append(traj_mod)