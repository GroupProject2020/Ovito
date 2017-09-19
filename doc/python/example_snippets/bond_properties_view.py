from ovito.io import import_file
from ovito.data import BondProperty, Bonds
from ovito.modifiers import ComputeBondLengthsModifier
import numpy
pipeline = import_file('bonds.data.gz', atom_style = 'bond')
pipeline.modifiers.append(ComputeBondLengthsModifier())
# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data = pipeline.compute()

bond_lengths = data.bond_properties['Length']
has_selection = 'Selection' in data.bond_properties
name_list = data.bond_properties.keys()
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
num_bonds = len(data.expect(Bonds))
colors = numpy.random.random_sample(size = (num_bonds, 3))
data.bond_properties.create('Color', data=colors)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
prop = data.bond_properties.create('Color')
with prop.modify() as arr:
    arr[...] = numpy.random.random_sample(size = prop.shape)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
values = numpy.arange(0, num_bonds, dtype=int)
data.bond_properties.create('myint', data=values)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
prop = data.bond_properties.create('myvector', dtype=float, components=3)
with prop.modify() as arr:
    arr[...] = numpy.random.random_sample(size = prop.shape)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
