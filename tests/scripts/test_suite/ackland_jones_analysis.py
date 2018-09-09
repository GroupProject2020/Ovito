from ovito.io import *
from ovito.modifiers import AcklandJonesModifier

import numpy

node = import_file("../../files/CFG/fcc_coherent_twin.0.cfg")
modifier = AcklandJonesModifier()
node.modifiers.append(modifier)

modifier.structures[AcklandJonesModifier.Type.FCC].color = (1,0,0)

node.compute()
print("Computed structure types:")
print(node.output.particle_properties.structure_type.array)

assert(node.output.attributes["AcklandJones.counts.FCC"] == 128)
assert(node.output.particle_properties.structure_type.array[0] == 1)
assert(node.output.particle_properties.structure_type.array[0] == AcklandJonesModifier.Type.FCC)
assert((node.output.particle_properties.color.array[0] == (1,0,0)).all())
assert(AcklandJonesModifier.Type.OTHER == 0)
assert(AcklandJonesModifier.Type.FCC == 1)
assert(AcklandJonesModifier.Type.HCP == 2)
assert(AcklandJonesModifier.Type.BCC == 3)
assert(AcklandJonesModifier.Type.ICO == 4)