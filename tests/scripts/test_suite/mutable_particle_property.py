from ovito.io import *
from ovito.modifiers import *
from ovito.data import *
import numpy as np

pipeline = import_file("../../files/CFG/shear.void.120.cfg")
modifier = CommonNeighborAnalysisModifier()
pipeline.modifiers.append(modifier)
data = pipeline.compute()

print("Number of FCC atoms: {}".format(data.attributes['CommonNeighborAnalysis.counts.FCC']))

pos_property = data.particles['Position']
print(pos_property.array)
print(pos_property.marray)
pos_property.marray[0] = (0,0,0)
print(pos_property.array)
assert(pos_property.array[0,0] == 0.0)
assert(pos_property[0,0] == 0.0)