from ovito.io import import_file
from ovito.data import BondProperty
from ovito.modifiers import ComputeBondLengthsModifier
import numpy
pipeline = import_file('input/bonds.data.gz', atom_style = 'bond')
pipeline.modifiers.append(ComputeBondLengthsModifier())
# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data = pipeline.compute()

bond_lengths = data.bonds['Length']
has_selection = 'Selection' in data.bonds
name_list = data.bonds.keys()
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
colors = numpy.random.random_sample(size = (data.bonds.count, 3))
data.bonds.create_property('Color', data=colors)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
prop = data.bonds.create_property('Color')
with prop as arr:
    arr[...] = numpy.random.random_sample(size = prop.shape)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
values = numpy.arange(0, data.bonds.count, dtype=int)
data.bonds.create_property('myint', data=values)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
prop = data.bonds.create_property('myvector', dtype=float, components=3)
with prop:
    prop[...] = numpy.random.random_sample(size = prop.shape)
# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
