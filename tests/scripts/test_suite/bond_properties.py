from ovito.io import import_file
from ovito.data import Bonds
import numpy as np

pipeline = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')
data = pipeline.compute()

assert(isinstance(data.bonds, Bonds))
assert(data.bonds is data.particles.bonds)
assert(data.bonds.count > 0)

# Set the display color of bond type 1.
data.bonds.vis.use_particle_colors = False
def init(frame, data):
    data.bonds_['Bond Type_'].types[0].color = (0.4, 0.1, 0.9)
pipeline.modifiers.append(init)

# Check if color property values are initialized correctly.
color_property = data.bonds_.create_property("Color")
assert(color_property.shape == (data.bonds.count, 3))
print(color_property[...])
#assert(np.all(color_property == data.bonds.vis.color))
