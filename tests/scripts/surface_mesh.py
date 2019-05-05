from ovito.io import import_file, export_file
from ovito.modifiers import ConstructSurfaceModifier
import numpy as np

import sys
import os

if "ovito.plugins.CrystalAnalysisPython" not in sys.modules:
    print("Skipping this test, because CrystalAnalysis module is not present")
    sys.exit()

pipeline = import_file("../files/CFG/lammps_dumpi-42-1100-510000.cfg")
pipeline.modifiers.append(ConstructSurfaceModifier(radius = 3.8, smoothing_level = 2))

# Write a modifier function that modifies a SurfaceMesh
def modify_surface(frame, data):
    surface_mesh = data.make_mutable(data.surface)

    surface_mesh.get_cutting_planes()
    surface_mesh.get_face_adjacency()
    surface_mesh.get_faces()
    surface_mesh.get_vertices()
    surface_mesh.set_cutting_planes([[0,1,3,0.4]])
pipeline.modifiers.append(modify_surface)

# Verify pipeline results
data = pipeline.compute()
surface_mesh = data.surface
assert(len(surface_mesh.get_cutting_planes()) == 1)

# Test VTK file export.
export_file(surface_mesh, "_surface_mesh.vtk", "vtk/trimesh")
os.remove("_surface_mesh.vtk")

