from ovito.io import *
from ovito.data import *
import numpy

pipeline = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')
data = pipeline.compute()

data.create_bond_property(BondProperty.Type.Color)
data.create_user_bond_property("MyProperty", "int", 2)
values = numpy.ones(data.number_of_full_bonds)
data.create_user_bond_property("MyProperty2", "float", 1, values)

print("Number of data objects: ", len(data.objects))
print(data.bond_properties)
print(data.bond_properties.bond_type)
print(list(data.bond_properties.keys()))
print(list(data.bond_properties.values()))
print(data.bond_properties["Bond Type"])
print(data.bond_properties.bond_type.array)
print(data.bond_properties["MyProperty2"].array)
