from ovito.io import *
from ovito.data import *

# Tests access to BondType objects.

pipeline = import_file("../../files/LAMMPS/class2.data", atom_style = 'full')
tprop = pipeline.source.data.bonds.bond_types

for t in tprop.types:
    t.name = "MyName" + str(t.id)
    print(t.id, t.name)

assert(len(tprop.types) == 26)
assert(tprop.types[0].id == 1)

print(tprop.types[0].id)
print(tprop.types[0].color)
print(tprop.types[0].name)

assert(tprop.type_by_id(1).id == 1)
assert(tprop.type_by_id(tprop.types[1].id) == tprop.types[1])
assert(tprop.type_by_id(tprop.types[2].id) == tprop.types[2])

assert(tprop.type_by_name(tprop.types[1].name) == tprop.types[1])
assert(tprop.type_by_name(tprop.types[2].name) == tprop.types[2])
