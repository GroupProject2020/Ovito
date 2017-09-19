from ovito.io import import_file
from ovito.modifiers import CreateBondsModifier
from ovito.data import Bonds, SimulationCell

# Load a set of atoms and create bonds between pairs of atoms that are within 
# a given cutoff distance of each other using the Create Bonds modifier.
pipeline = import_file('trajectory.dump')
pipeline.modifiers.append(CreateBondsModifier(cutoff = 3.4))

# Evaluate pipeline and retrieve the Bonds data object.
bonds = pipeline.compute().expect(Bonds)
print("Number of generated bonds: ", len(bonds))

for i in range(len(bonds)):
    print('Bond %i from atom %i to atom %i' % (i, bonds[i,0], bonds[i,1]))

from ovito.vis import BondsDisplay
import numpy

# Configure visual appearance of bonds:
bonds.display.enabled = True
bonds.display.shading = BondsDisplay.Shading.Flat
bonds.display.width = 0.3

# Computing bond vectors.
data = pipeline.compute()
bonds = data.expect(Bonds)
particle_positions = data.particle_properties['Position']
bond_vectors = particle_positions[bonds[:,1]] - particle_positions[bonds[:,0]]
cell = data.expect(SimulationCell)
bond_vectors += numpy.dot(cell[:,:3], bonds.pbc_vectors.T).T
