from ovito.io import *
from ovito.modifiers import *

import numpy

pipeline = import_file("../files/LAMMPS/animation.dump.gz")
pipeline.modifiers.append(SelectExpressionModifier(expression = "ParticleIndex<5"))

modifier = ColorCodingModifier(particle_property = "Position.X", only_selected = True)
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  start_value: {}".format(modifier.start_value))
print("  end_value: {}".format(modifier.end_value))
print("  gradient: {}".format(modifier.gradient))
print("  only_selected: {}".format(modifier.only_selected))
print("  assign_to: {}".format(modifier.assign_to))
print("  particle_property: {}".format(modifier.particle_property))
print("  bond_property: {}".format(modifier.bond_property))
print("  property: {}".format(modifier.property))
print("  operate_on: {}".format(modifier.operate_on))

modifier.operate_on = 'bonds'
modifier.operate_on = 'vectors'
modifier.operate_on = 'particles'
modifier.property = 'Position.X'

modifier.gradient = ColorCodingModifier.Rainbow()
modifier.gradient = ColorCodingModifier.Jet()
modifier.gradient = ColorCodingModifier.Hot()
modifier.gradient = ColorCodingModifier.Grayscale()
modifier.gradient = ColorCodingModifier.BlueWhiteRed()
modifier.gradient = ColorCodingModifier.Viridis()
modifier.gradient = ColorCodingModifier.Magma()
modifier.gradient = ColorCodingModifier.Image("../../doc/manual/images/modifiers/color_coding_custom_map.png")

# Backward compatibility with OVITO 2.9.0:
modifier.gradient = ColorCodingModifier.Custom("../../doc/manual/images/modifiers/color_coding_custom_map.png")

print(pipeline.compute().particles.color is not None)

modifier.particle_property = "Position.X"
data = pipeline.compute()

print("Particle properties:", data.particles)
assert('Selection' in data.particles)

modifier = ColorCodingModifier(
    assign_to = ColorCodingModifier.AssignmentMode.Bonds,
    bond_property = "Length",
    start_value = 0.0,
    end_value = 0.0,
    only_selected = True,
    gradient = ColorCodingModifier.Grayscale()
)
assert(modifier.property == 'Length')
assert(modifier.operate_on == 'bonds')


# Test operation on voxel grids.
pipeline = import_file("../files/POSCAR/CHGCAR.nospin.gz")
# Add another voxel grid.
pipeline.modifiers.append(SpatialBinningModifier(direction = SpatialBinningModifier.Direction.XY, property = "Position.X"))
modifier = ColorCodingModifier(
        operate_on = "voxels",
        property = "Charge density"
    )
pipeline.modifiers.append(modifier)
# Check if the 'Color' property has been created by the modifier.
data = pipeline.compute()
print(data.grids['charge-density']['Color'][...])
# Check if we can control which voxel grid the modifier operates on.
modifier.operate_on = "voxels:binning[Position.X]"
modifier.property = "Position.X"
assert(modifier.operate_on == "voxels:binning[Position.X]")
data = pipeline.compute()
assert(not "Color" in data.grids['charge-density'])
print(data.grids['binning[Position.X]']['Color'][...])
# Do the counter check: activate coloring of the other voxel grid.
modifier.property = "Charge density"
modifier.operate_on = "voxels:charge-density"
data = pipeline.compute()
assert(not "Color" in data.grids['binning[Position.X]'])
print(data.grids['charge-density']['Color'][...])
