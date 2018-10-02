from ovito.io import import_file
from ovito.data import SimulationCell
from ovito.modifiers import ComputePropertyModifier
from ovito.vis import BondsVis
pipeline = import_file('input/bonds.data.gz', atom_style = 'bond')
pipeline.modifiers.append(ComputePropertyModifier(operate_on = 'bonds', output_property = 'Length', expressions = ['BondLength']))

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data = pipeline.compute()
print("Number of bonds:", data.particles.bonds.count)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
print("Bond property names:")
print(data.particles.bonds.keys())
if 'Length' in data.particles.bonds:
    length_prop = data.particles.bonds['Length']
    assert(len(length_prop) == data.particles.bonds.count)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
for a,b in data.particles.bonds.topology:
    print("Bond from particle %i to particle %i" % (a,b))
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data.particles.bonds.vis.enabled = True
data.particles.bonds.vis.shading = BondsVis.Shading.Flat
data.particles.bonds.vis.width = 0.3
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

import numpy
# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
topology = data.particles.bonds.topology
positions = data.particles.positions
bond_vectors = positions[topology[:,1]] - positions[topology[:,0]]
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
cell = data.cell
bond_vectors += numpy.dot(cell[:3,:3], data.particles.bonds.pbc_vectors.T).T
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<