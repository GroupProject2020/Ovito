from ovito.io import import_file
from ovito.data import CutoffNeighborFinder

# Load input simulation file.
pipeline = import_file("simulation.dump")
data = pipeline.compute()
positions = data.particle_properties['Position']

# Initialize neighbor finder object:
cutoff = 3.5
finder = CutoffNeighborFinder(cutoff, data)

# Loop over all particles:
for index in range(len(positions)):
    print("Neighbors of particle %i:" % index)
    # Iterate over the neighbors of the current particle:
    for neigh in finder.find(index):
        print(neigh.index, neigh.distance, neigh.delta, neigh.pbc_shift)