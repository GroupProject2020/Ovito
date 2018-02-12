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

print("  vis: {}".format(modifier.vis))

node.compute()
print("Output:")

print(node.output.surface)
