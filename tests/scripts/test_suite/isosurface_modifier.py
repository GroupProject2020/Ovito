from ovito.io import *
from ovito.modifiers import *

node = import_file("../../files/POSCAR/CHGCAR.nospin.gz")

modifier = CreateIsosurfaceModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  isolevel: {}".format(modifier.isolevel))
modifier.isolevel = 0.02

print("  property: {}".format(modifier.property))
modifier.property = "Charge density"

print("  mesh_display: {}".format(modifier.mesh_display))

node.compute()
print("Output:")

print(node.output.surface)
