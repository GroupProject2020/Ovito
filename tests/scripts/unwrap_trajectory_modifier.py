from ovito.io import import_file
from ovito.modifiers import UnwrapTrajectoriesModifier, ComputePropertyModifier
import numpy as np

pipeline = import_file("../files/GSD/image_flags.gsd")
pipeline.modifiers.append(ComputePropertyModifier(operate_on = "bonds", output_property = "length1", expressions = ["BondLength"]))
pipeline.modifiers.append(UnwrapTrajectoriesModifier())
pipeline.modifiers.append(ComputePropertyModifier(operate_on = "bonds", output_property = "length2", expressions = ["BondLength"]))

data1 = pipeline.source.compute()
data2 = pipeline.compute()

assert(not np.all(data1.particles.bonds["Periodic Image"] == [0,0,0]))
assert(np.all(data2.particles.bonds["Periodic Image"] == [0,0,0]))
assert(np.allclose(data2.particles.bonds["length1"], data2.particles.bonds["length2"]))
