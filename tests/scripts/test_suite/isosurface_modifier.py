from ovito.io import import_file
from ovito.modifiers import CreateIsosurfaceModifier

pipeline = import_file("../../files/POSCAR/CHGCAR.nospin.gz")
data = pipeline.compute()
print(list(data.grids.keys()))

modifier = CreateIsosurfaceModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  isolevel: {}".format(modifier.isolevel))
modifier.isolevel = 0.02

print("  operate_on: {}".format(modifier.operate_on))
modifier.operate_on = "voxels:"

print("  property: {}".format(modifier.property))
modifier.property = "Charge density"

print("  vis: {}".format(modifier.vis))

data = pipeline.compute()

surface_mesh = data.surfaces['isosurface']
assert(surface_mesh.vis is modifier.vis)

# Add a second isosurface modifer:
modifier2 = CreateIsosurfaceModifier(property = "Charge density")
pipeline.modifiers.append(modifier2)
data = pipeline.compute()

surface_mesh2 = data.surfaces['isosurface.2']
assert(surface_mesh2.vis is modifier2.vis)
