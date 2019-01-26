import ovito
import ovito.io
import ovito.modifiers

import numpy

pipeline = ovito.io.import_file("../files/CFG/shear.void.120.cfg")
pipeline.modifiers.append(ovito.modifiers.CommonNeighborAnalysisModifier())

modifier = ovito.modifiers.SelectTypeModifier()
pipeline.modifiers.append(modifier)

modifier.types = {1,2}
print("types:", modifier.types)

print("property:", modifier.property)
data = pipeline.compute()
print("number of particles:", len(data.particle_properties['Selection']))
nselected = numpy.count_nonzero(data.particle_properties['Selection'])
print("num selected:", nselected)
assert(nselected == 1444)
assert(nselected == data.attributes['SelectType.num_selected'])

modifier.property = "Structure Type"
modifier.types = { ovito.modifiers.CommonNeighborAnalysisModifier.Type.FCC, 
                   ovito.modifiers.CommonNeighborAnalysisModifier.Type.HCP }
print("types:", modifier.types)
data = pipeline.compute()
nselected = numpy.count_nonzero(data.particle_properties['Selection'])
print("num selected:", nselected)
assert(nselected == 1199)
assert(nselected == data.attributes['SelectType.num_selected'])

print("types:", modifier.types)
assert(len(modifier.types) == 2)
assert(2 in modifier.types)
assert(not 3 in modifier.types)
modifier.types.add(3)
assert(3 in modifier.types)
assert(not 'test' in modifier.types)
modifier.types.add('test')
assert('test' in modifier.types)
assert(len(modifier.types) == 4)
print("types:", modifier.types)

modifier.types.remove('test')
print("types:", modifier.types)
assert(len(modifier.types) == 3)

data = pipeline.compute()
