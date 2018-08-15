import ovito
import ovito.io
import ovito.modifiers

import numpy as np

pipeline = ovito.io.import_file("../../files/CFG/shear.void.120.cfg")
modifier = ovito.modifiers.ComputePropertyModifier()
pipeline.modifiers.append(modifier)

print(modifier.expressions)
print(modifier.only_selected)
print(modifier.output_property)

assert(modifier.operate_on == "particles")
assert(len(modifier.expressions) == 1)
assert(modifier.expressions[0] == "0")

# Test: Compute a simple custom vector property. 
modifier.output_property = 'testprop' 
modifier.expressions = ["Position.X * 2.0", "Position.Y / 2"]
modifier.only_selected = False
data = pipeline.compute()
assert('testprop' in data.particles)
expected_result = np.transpose([data.particles['Position'][:,0] * 2, data.particles['Position'][:,1] / 2])
assert(np.array_equal(data.particles['testprop'], expected_result))

# Test: "NumNeighbors" expression
modifier.output_property = 'neighprop' 
modifier.expressions = ["NumNeighbors"]
modifier.neighbor_mode = True
modifier.cutoff_radius = 3.2
pipeline.modifiers.append(ovito.modifiers.CoordinationAnalysisModifier(cutoff = modifier.cutoff_radius))
data = pipeline.compute()
expected_result = data.particles['Coordination']
assert(np.array_equal(data.particles['neighprop'], expected_result))

# Test: neighbor_expressions
modifier.output_property = 'neighprop' 
modifier.expressions = ["0"]
modifier.neighbor_expressions = ["1"]
data = pipeline.compute()
assert(np.array_equal(data.particles['neighprop'], expected_result))

# Test: bond property computation
pipeline.modifiers.insert(0, ovito.modifiers.CreateBondsModifier(cutoff = 3.2))
modifier.operate_on = 'bonds' 
modifier.output_property = 'testprop' 
modifier.expressions = ["@1.ParticleType != @2.ParticleType"]
data = pipeline.compute()
expected_result = data.particles['Particle Type'][data.bonds['Topology'][:,0]] != data.particles['Particle Type'][data.bonds['Topology'][:,1]]
assert(np.array_equal(data.bonds['testprop'], expected_result))

# Test: bond length computation
pipeline.source.load("../../files/XSF/1symb.xsf")
pipeline.modifiers.append(ovito.modifiers.ComputePropertyModifier( 
    expressions = ['sqrt((@1.Position.X-@2.Position.X)^2 + (@1.Position.Y-@2.Position.Y)^2 + (@1.Position.Z-@2.Position.Z)^2)'],
    output_property = 'MyLength',
    operate_on = 'bonds'))
modifier.output_property = 'Length' 
modifier.expressions = ["BondLength"]
data = pipeline.compute()
expected_result = data.bonds['MyLength']
assert(np.allclose(data.bonds['Length'], expected_result))
