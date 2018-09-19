from ovito.io import import_file
import numpy as np

pipeline = import_file("../../files/CFG/shear.void.120.cfg")
data = pipeline.compute()

pos_property = data.particles_['Position_']
with pos_property:
    pos_property[0] = (0.2,0.4,0.6)
assert(np.all(pos_property[0] == (0.2, 0.4, 0.6)))

# Backward compatibility with OVITO 2.9.0:
pos_property.marray[0] = (0.1, 0.2, 0.3)
assert(np.all(pos_property.array[0] == (0.1, 0.2, 0.3)))
