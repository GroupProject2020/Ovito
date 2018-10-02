from ovito.io import import_file
from ovito.data import Bonds
import numpy as np

pipeline = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')

# Assign a special color to bond type 1.
my_bond_color = (0.3, 0.1, 0.8)
pipeline.source.data.bonds.vis.use_particle_colors = False
pipeline.source.data.bonds.bond_types.types[0].color = my_bond_color

# Evaluate pipeline
data = pipeline.compute()
assert(isinstance(data.bonds, Bonds))
assert(data.bonds is data.particles.bonds)
assert(data.bonds.count > 0)

# Check if values of newly created color property get initialized correctly according 
# to the settings of the vis element.
color_property = data.bonds_.create_property("Color")
assert(color_property.shape == (data.bonds.count, 3))
assert(np.all(color_property == my_bond_color))
