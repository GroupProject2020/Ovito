from ovito.io import *
from ovito.data import *
from ovito.modifiers import *

import numpy

node = import_file("../../files/CFG/fcc_coherent_twin.0.cfg")
modifier = VoroTopModifier()

print("Parameter defaults:")
print("  only_selected: {}".format(modifier.only_selected))
print("  use_radii: {}".format(modifier.use_radii))
print("  filter_file: {}".format(modifier.filter_file))

modifier.use_radii = True
modifier.filter_file = "../../files/VoroTop/FCC-BCC-ICOS-both-HCP.gz"

node.modifiers.append(modifier)

modifier.structures[4].color = (1,0,0)
assert(modifier.structures[4].name == "FCC-HCP")

node.compute()

print("Computed structure types:")
print(node.output.particle_properties.structure_type.array)

assert(node.output.particle_properties.structure_type.array[0] == 4)
assert((node.output.particle_properties.color.array[0] == (1,0,0)).all())
