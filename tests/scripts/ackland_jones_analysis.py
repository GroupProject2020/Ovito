from ovito.io import *
from ovito.modifiers import AcklandJonesModifier

import numpy

pipeline = import_file("../files/CFG/fcc_coherent_twin.0.cfg")
modifier = AcklandJonesModifier()
pipeline.modifiers.append(modifier)

modifier.structures[AcklandJonesModifier.Type.FCC].color = (1,0,0)

data = pipeline.compute()
print("Computed structure types:")
print(data.particles.structure_types[...])

assert(data.attributes["AcklandJones.counts.FCC"] == 128)
assert(data.particles.structure_types[0] == 1)
assert(data.particles.structure_types[0] == AcklandJonesModifier.Type.FCC)
assert((data.particles.colors[0] == (1,0,0)).all())
assert(AcklandJonesModifier.Type.OTHER == 0)
assert(AcklandJonesModifier.Type.FCC == 1)
assert(AcklandJonesModifier.Type.HCP == 2)
assert(AcklandJonesModifier.Type.BCC == 3)
assert(AcklandJonesModifier.Type.ICO == 4)