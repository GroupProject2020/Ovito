from ovito.io import *
from ovito.data import *
from ovito.modifiers import *
import numpy

pipeline = import_file("../files/CFG/SiVacancy.cfg")

modifier = ChillPlusModifier(cutoff = 3.5)
pipeline.modifiers.append(modifier)

print("Parameter defaults:")
print("  cutoff: {}".format(modifier.cutoff))
print("  only_selected: {}".format(modifier.only_selected))
print("  color_by_type: {}".format(modifier.color_by_type))

modifier.structures[ChillPlusModifier.Type.HEXAGONAL_ICE].color = (1,0,0)

data = pipeline.compute()

print("Computed structure types:")
print(data.particles.structure_types[...])
print("Number of particles: {}".format(data.particles.count))
print("Number of OTHER particles: {}".format(data.attributes['ChillPlus.counts.OTHER']))
print("Number of HEXAGONAL_ICE particles: {}".format(data.attributes['ChillPlus.counts.HEXAGONAL_ICE']))
print("Number of CUBIC_ICE particles: {}".format(data.attributes['ChillPlus.counts.CUBIC_ICE']))
print("Number of INTERFACIAL_ICE particles: {}".format(data.attributes['ChillPlus.counts.INTERFACIAL_ICE']))
print("Number of HYDRATE particles: {}".format(data.attributes['ChillPlus.counts.HYDRATE']))
print("Number of INTERFACIAL_HYDRATE particles: {}".format(data.attributes['ChillPlus.counts.INTERFACIAL_HYDRATE']))

assert(data.attributes['ChillPlus.counts.CUBIC_ICE'] == 1723)
assert(numpy.count_nonzero(data.particles.structure_types == ChillPlusModifier.Type.CUBIC_ICE) == data.attributes['ChillPlus.counts.CUBIC_ICE'])
assert((data.particles.color[data.particles.structure_types == ChillPlusModifier.Type.CUBIC_ICE] == (1,0,0)).all())

assert(ChillPlusModifier.Type.OTHER == 0)
assert(ChillPlusModifier.Type.HEXAGONAL_ICE == 1)
assert(ChillPlusModifier.Type.CUBIC_ICE == 2)
assert(ChillPlusModifier.Type.INTERFACIAL_ICE == 3)
assert(ChillPlusModifier.Type.HYDRATE == 4)
assert(ChillPlusModifier.Type.INTERFACIAL_HYDRATE == 5)
