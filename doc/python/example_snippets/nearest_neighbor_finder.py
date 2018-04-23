from ovito.io import import_file
from ovito.data import NearestNeighborFinder

# Load input simulation file.
pipeline = import_file("input/simulation.dump")
data = pipeline.compute()

# Initialize neighbor finder object.
# Visit the 12 nearest neighbors of each particle.
N = 12
finder = NearestNeighborFinder(N, data)

# Prefetch the property array containing the particle type information:
ptypes = data.particles['Particle Type']

# Loop over all input particles:
for index in range(data.particles.count):
    print("Nearest neighbors of particle %i:" % index)
    # Iterate over the neighbors of the current particle, starting with the closest:
    for neigh in finder.find(index):
        print(neigh.index, neigh.distance, neigh.delta)
        # The index can be used to access properties of the current neighbor, e.g.
        type_of_neighbor = ptypes[neigh.index]

# Find particles closest to some spatial point (x,y,z):
coords = (0, 0, 0)
for neigh in finder.find_at(coords):
    print(neigh.index, neigh.distance, neigh.delta)
