from ovito.io import *
from ovito.modifiers import *
import numpy as np

pipeline = import_file("../../files/CFG/shear.void.120.cfg")
pipeline.add_to_scene()

pipeline.modifiers.append(SliceModifier(
    distance = -12,
    inverse = True,
    slab_width = 18.0
))

pipeline.modifiers.append(SliceModifier(
    distance = 12,
    inverse = True,
    slab_width = 18.0
))

modifier = ClusterAnalysisModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  cutoff: {}".format(modifier.cutoff))
modifier.cutoff = 2.8
print("  neighbor_mode: {}".format(modifier.neighbor_mode))
modifier.neighbor_mode = ClusterAnalysisModifier.NeighborMode.CutoffRange
print("  sort_by_size: {}".format(modifier.sort_by_size))
modifier.sort_by_size = False

data = pipeline.compute()

print("Output:")
print("Number of clusters: {}".format(data.attributes['ClusterAnalysis.cluster_count']))
assert(data.attributes['ClusterAnalysis.cluster_count'] == 2)
print(data.particles['Cluster'][...])

modifier.sort_by_size = True
data = pipeline.compute()
print(data.attributes['ClusterAnalysis.largest_size'])
assert(data.attributes['ClusterAnalysis.largest_size'] >= 1)

pipeline.modifiers.insert(2, VoronoiAnalysisModifier(generate_bonds=True))
modifier.neighbor_mode = ClusterAnalysisModifier.NeighborMode.Bonding
data = pipeline.compute()
assert(data.attributes['ClusterAnalysis.largest_size'] == data.particles.count)