import sys
if "ovito.modifiers.crystalanalysis" not in sys.modules: sys.exit()
############## Code snippet begins here ################
from ovito.io import import_file, export_file
from ovito.data import SurfaceMesh, SimulationCell
from ovito.modifiers import ConstructSurfaceModifier

# Load a particle structure and reconstruct its geometric surface:
pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(ConstructSurfaceModifier(radius = 2.9))
data = pipeline.compute()
mesh = data.surfaces['surface']

# Query computed surface properties:
print("Surface area: %f" % data.attributes['ConstructSurfaceMesh.surface_area'])
print("Solid volume: %f" % data.attributes['ConstructSurfaceMesh.solid_volume'])
fraction = data.attributes['ConstructSurfaceMesh.solid_volume'] / data.cell.volume
print("Solid volume fraction: %f" % fraction)

# Export the surface triangle mesh to a VTK file.
export_file(mesh, "output/surface.vtk", "vtk/trimesh")
