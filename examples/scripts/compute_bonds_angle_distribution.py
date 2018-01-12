from ovito.io import *
from ovito.data import *
from ovito.modifiers import *
import numpy as np

node = import_file('CuZr_metallic_glass.dump.gz')

# Number of bins of the histogram to create
bin_count = 100
angle_cosine_histogram = np.zeros((bin_count,), int)

# Create bonds
create_bonds_mod = CreateBondsModifier(mode=CreateBondsModifier.Mode.Pairwise)
create_bonds_mod.set_pairwise_cutoff('Type 1', 'Type 1', 2.7)
create_bonds_mod.set_pairwise_cutoff('Type 1', 'Type 2', 2.9)
create_bonds_mod.set_pairwise_cutoff('Type 2', 'Type 2', 3.3)
node.modifiers.append(create_bonds_mod)

# Compute normalized bond vectors
data = node.compute()
particle_positions = data.particle_properties.position.array
bonds_array = data.bonds.array
bond_vectors = particle_positions[bonds_array[:,1]] - particle_positions[bonds_array[:,0]]
bond_vectors += np.dot(data.cell.matrix[:,:3], data.bonds.pbc_vectors.T).T
bond_vectors /= np.linalg.norm(bond_vectors, axis=1).reshape(-1,1)

print("Number of particles:", data.number_of_particles)
print("Number of bonds:", data.number_of_bonds)

# Iterate over all particles and their bonds
bonds_enum = Bonds.Enumerator(data.bonds)
for particle_index in range(data.number_of_particles):
    
    # Build local list of half-bonds of the current particle
    local_bonds = bond_vectors[np.fromiter(bonds_enum.bonds_of_particle(particle_index), np.intp)]
    
    # Compute bond angle cosines of all bond pairs
    angle_matrix = np.squeeze(np.inner(local_bonds[np.newaxis,:,:], local_bonds[:,np.newaxis,:]), axis=(0,3))

    # Create linear array of bond angle cosines (all values in the upper half of matrix)
    angle_cosines = angle_matrix[np.triu_indices(len(angle_matrix), k=1)]

    # Incrementally compute histogram of angle cosines
    angle_cosine_histogram += np.histogram(angle_cosines, bins=bin_count, range=(-1.0,1.0))[0]

print("Bond angle cosine histogram:")
print(angle_cosine_histogram)