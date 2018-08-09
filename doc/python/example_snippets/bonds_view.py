from ovito.io import import_file
from ovito.data import BondProperty, SimulationCell
from ovito.modifiers import ComputePropertyModifier
from ovito.vis import BondsVis
pipeline = import_file('input/bonds.data.gz', atom_style = 'bond')
pipeline.modifiers.append(ComputePropertyModifier(operate_on = 'bonds', output_property = 'Length', expressions = ['BondLength']))

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data = pipeline.compute()
print("Number of bonds:", data.bonds.count)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
print("Bond property names:")
print(data.bonds.keys())
if 'Length' in data.bonds:
    length_prop = data.bonds['Length']
    assert(len(length_prop) == data.bonds.count)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
topology = data.bonds['Topology']
for a,b in topology:
    print("Bond from particle %i to particle %i" % (a,b))
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data.bonds['Topology'].vis.enabled = True
data.bonds['Topology'].vis.shading = BondsVis.Shading.Flat
data.bonds['Topology'].vis.width = 0.3
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

import numpy
# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
topology = data.bonds['Topology']
positions = data.particles['Position']
bond_vectors = positions[topology[:,1]] - positions[topology[:,0]]
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
cell = data.expect(SimulationCell)
bond_vectors += numpy.dot(cell[:,:3], data.bonds['Periodic Image'].T).T
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<