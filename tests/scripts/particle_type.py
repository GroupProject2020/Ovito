from ovito.io import *
from ovito.data import *
from ovito.modifiers import *

# Tests access to a ParticleType objects

pipeline = import_file("../files/CFG/shear.void.120.cfg")
tprop = pipeline.compute().particles['Particle Type']

assert(len(tprop.types) == 3)
assert(tprop.types[0].id == 1)

print(tprop.types[0].id)
print(tprop.types[0].color)
print(tprop.types[0].name)
print(tprop.types[0].radius)

assert(tprop.type_by_id(1).id == 1)
assert(tprop.type_by_id(tprop.types[1].id) == tprop.types[1])
assert(tprop.type_by_id(tprop.types[2].id) == tprop.types[2])

assert(tprop.type_by_name(tprop.types[1].name) == tprop.types[1])
assert(tprop.type_by_name(tprop.types[2].name) == tprop.types[2])

# Let the CNA modifier create a structural type property.
pipeline.modifiers.append(CommonNeighborAnalysisModifier())
sprop = pipeline.compute().particles['Structure Type']

assert(len(sprop.types) >= 1)
assert(sprop.type_by_id(CommonNeighborAnalysisModifier.Type.HCP).id == CommonNeighborAnalysisModifier.Type.HCP)

# Backward compatibility check:
assert(len(tprop.type_list) == 3)
assert(tprop.get_type_by_id(tprop.type_list[1].id) == tprop.type_list[1])
assert(tprop.get_type_by_name(tprop.type_list[1].name) == tprop.type_list[1])
