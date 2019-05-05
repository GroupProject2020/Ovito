from ovito.io import import_file
from ovito.modifiers import ConstructSurfaceModifier
import numpy as np
import sys

if "ovito.plugins.CrystalAnalysisPython" not in sys.modules:
    print("Skipping this test, because CrystalAnalysis module is not present")
    sys.exit()

pipeline = import_file("../files/CFG/lammps_dumpi-42-1100-510000.cfg")

modifier = ConstructSurfaceModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  only_selected: {}".format(modifier.only_selected))
modifier.only_selected = False

print("  radius: {}".format(modifier.radius))
modifier.radius = 3.8

print("  smoothing_level: {}".format(modifier.smoothing_level))
modifier.smoothing_level = 0

print("  cap_color: {}".format(modifier.vis.cap_color))
print("  cap_transparency: {}".format(modifier.vis.cap_transparency))
print("  show_cap: {}".format(modifier.vis.show_cap))
print("  smooth_shading: {}".format(modifier.vis.smooth_shading))
print("  surface_color: {}".format(modifier.vis.surface_color))
print("  surface_transparency: {}".format(modifier.vis.surface_transparency))

data = pipeline.compute()

print("Outputs:")
print("  solid_volume= {}".format(data.attributes['ConstructSurfaceMesh.solid_volume']))
print("  surface_area= {}".format(data.attributes['ConstructSurfaceMesh.surface_area']))

surface_mesh = data.surface
assert(surface_mesh is data.surfaces['surface'])

assert(surface_mesh.locate_point((0,0,0)) == 0) # Exterior point
assert(surface_mesh.locate_point((82.9433, -43.5068, 26.4005), eps=1e-1) == -1) # On boundary
assert(surface_mesh.locate_point((80.0, -47.2837, 26.944)) == 1) # Interior point
