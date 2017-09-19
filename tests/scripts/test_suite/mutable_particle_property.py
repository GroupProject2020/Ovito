from ovito.io import *
from ovito.modifiers import *
from ovito.data import *
from ovito.pipeline import StaticSource
import numpy as np

node = import_file("../../files/CFG/shear.void.120.cfg")
modifier = CommonNeighborAnalysisModifier()
node.modifiers.append(modifier)
node.compute()

print("Number of FCC atoms: {}".format(node.output.attributes['CommonNeighborAnalysis.counts.FCC']))

print(node.source.particle_properties.position.array)
print(node.source.particle_properties.position.marray)
node.source.particle_properties.position.marray[0] = (0,0,0)
print(node.source.particle_properties.position.array)
assert(node.source.particle_properties.position.array[0,0] == 0.0)

node.source = StaticSource()