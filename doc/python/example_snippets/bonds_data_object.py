from ovito.io import import_file
from ovito.modifiers import CreateBondsModifier
from ovito.data import SimulationCell

# Load a set of atoms and create bonds between pairs of atoms that are within 
# a given cutoff distance of each other using the Create Bonds modifier.
pipeline = import_file('input/trajectory.dump')
pipeline.modifiers.append(CreateBondsModifier(cutoff = 3.4))

# Evaluate pipeline and retrieve bonds.
data = pipeline.compute()
print("Number of generated bonds: ", data.bonds.count)

for i in range(data.bonds.count):
    print('Bond %i from atom %i to atom %i' % (i, data.bonds[i,0], data.bonds[i,1]))

from ovito.vis import BondsVis
import numpy

# Configure visual appearance of bonds:
data.bonds['Topology'].vis.enabled = True
data.bonds['Topology'].vis.shading = BondsVis.Shading.Flat
data.bonds['Topology'].vis.width = 0.3

# Computing bond vectors.
data = pipeline.compute()
particle_positions = data.particles['Position']
bond_vectors = particle_positions[bonds[:,1]] - particle_positions[bonds[:,0]]
cell = data.expect(SimulationCell)
bond_vectors += numpy.dot(cell[:,:3], bonds.pbc_vectors.T).T
