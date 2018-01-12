import sys
if "ovito.modifiers.crystalanalysis" not in sys.modules: sys.exit()

from ovito.io import import_file
from ovito.data import SurfaceMesh, SimulationCell
from ovito.modifiers import ConstructSurfaceModifier

# Load a particle structure and reconstruct its geometric surface:
pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(ConstructSurfaceModifier(radius = 2.9))
data = pipeline.compute()
mesh = data.expect(SurfaceMesh)
cell = data.expect(SimulationCell)

# Query computed surface properties:
print("Surface area: %f" % data.attributes['ConstructSurfaceMesh.surface_area'])
print("Solid volume: %f" % data.attributes['ConstructSurfaceMesh.solid_volume'])
fraction = data.attributes['ConstructSurfaceMesh.solid_volume'] / cell.volume
print("Solid volume fraction: %f" % fraction)

# Export the surface triangle mesh to a VTK file.
mesh.export_vtk('output/surface.vtk', cell)
