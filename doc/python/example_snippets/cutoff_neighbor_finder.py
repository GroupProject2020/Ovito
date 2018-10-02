from ovito.io import import_file
from ovito.data import CutoffNeighborFinder

# Load input simulation file.
pipeline = import_file("input/simulation.dump")
data = pipeline.compute()

# Initialize neighbor finder object:
cutoff = 3.5
finder = CutoffNeighborFinder(cutoff, data)

# Prefetch the property array containing the particle type information:
ptypes = data.particles.particle_types

# Loop over all particles:
for index in range(data.particles.count):
    print("Neighbors of particle %i:" % index)

    # Iterate over the neighbors of the current particle:
    for neigh in finder.find(index):
        print(neigh.index, neigh.distance, neigh.delta, neigh.pbc_shift)

        # The index can be used to access properties of the current neighbor, e.g.
        type_of_neighbor = ptypes[neigh.index]