import sys
if "ovito.modifiers.crystalanalysis" not in sys.modules: sys.exit()

from ovito.io import import_file, export_file
from ovito.data import SurfaceMesh
from ovito.modifiers import ConstructSurfaceModifier

# Load a particle set and construct the surface mesh:
pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(ConstructSurfaceModifier(radius = 2.8))
mesh = pipeline.compute().surfaces['surface']

# Export the mesh to a VTK file for visualization with ParaView.
export_file(mesh, 'output/surface_mesh.vtk', 'vtk/trimesh')