from ovito.io import *
from ovito.modifiers import *
import numpy

pipeline = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')
pipeline.modifiers.append(WrapPeriodicImagesModifier())
data = pipeline.compute()
print(data.particles.bonds.pbc_vectors[...])
