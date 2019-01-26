from ovito.io import *
from ovito.modifiers import *
import numpy as np

pipeline = import_file("../files/CFG/shear.void.120.cfg")

modifier = CentroSymmetryModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  num_neighbors: {}".format(modifier.num_neighbors))
modifier.num_neighbors = 12

data = pipeline.compute()

print("Output:")
print(data.particles['Centrosymmetry'][...])
