from ovito.io import import_file
from ovito.modifiers import VoronoiAnalysisModifier
import numpy as np

pipeline = import_file("../../files/NetCDF/sheared_aSi.nc")

modifier = VoronoiAnalysisModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  compute_indices: {}".format(modifier.compute_indices))
modifier.compute_indices = True

print("  edge_threshold: {}".format(modifier.edge_threshold))
modifier.edge_threshold = 0.1

print("  face_threshold: {}".format(modifier.face_threshold))
modifier.face_threshold = 0.04

print("  relative_face_threshold: {}".format(modifier.relative_face_threshold))
modifier.relative_face_threshold = 0.01

print("  only_selected: {}".format(modifier.only_selected))
modifier.only_selected = False

print("  use_radii: {}".format(modifier.use_radii))
modifier.use_radii = True

data = pipeline.compute()

print("Output:")
print(data.particles["Atomic Volume"][...])
print(data.particles["Max Face Order"][...])
print(data.particles["Voronoi Index"][...])
print(data.particles["Coordination"][...])
print(data.attributes['Voronoi.max_face_order'])

# Consistency checks:
assert(data.attributes['Voronoi.max_face_order'] > 3)
assert(data.particles["Voronoi Index"].shape[1] == data.attributes['Voronoi.max_face_order'])
assert(np.max(data.particles["Max Face Order"]) == data.attributes['Voronoi.max_face_order'])
assert(np.sum(data.particles["Atomic Volume"]) == data.cell.volume)
