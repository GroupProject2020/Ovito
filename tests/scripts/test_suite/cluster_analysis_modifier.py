from ovito.io import *
from ovito.modifiers import *
import numpy as np

node = import_file("../../files/CFG/shear.void.120.cfg")
node.add_to_scene()

node.modifiers.append(SliceModifier(
    distance = -12,
    inverse = True,
    slab_width = 18.0
))

node.modifiers.append(SliceModifier(
    distance = 12,
    inverse = True,
    slab_width = 18.0
))

modifier = ClusterAnalysisModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  cutoff: {}".format(modifier.cutoff))
modifier.cutoff = 2.8
print("  neighbor_mode: {}".format(modifier.neighbor_mode))
modifier.neighbor_mode = ClusterAnalysisModifier.NeighborMode.CutoffRange
print("  sort_by_size: {}".format(modifier.sort_by_size))
modifier.sort_by_size = False

node.compute()

print("Output:")
print("Number of clusters: {}".format(node.output.attributes['ClusterAnalysis.cluster_count']))
assert(node.output.attributes['ClusterAnalysis.cluster_count'] == 2)
print(node.output.particle_properties.cluster)
print(node.output.particle_properties.cluster.array)

modifier.sort_by_size = True
node.compute()
print(node.output.attributes['ClusterAnalysis.largest_size'])
assert(node.output.attributes['ClusterAnalysis.largest_size'] >= 1)

node.modifiers.insert(2, VoronoiAnalysisModifier(generate_bonds=True))
modifier.neighbor_mode = ClusterAnalysisModifier.NeighborMode.Bonding
node.compute()
assert(node.output.attributes['ClusterAnalysis.largest_size'] == node.output.number_of_particles)