from ovito.io import import_file
from ovito.modifiers import ClusterAnalysisModifier
import numpy

pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(ClusterAnalysisModifier(cutoff = 2.8, sort_by_size = True))
data = pipeline.compute()

cluster_sizes = numpy.bincount(data.particle_properties['Cluster'])
numpy.savetxt("output/cluster_sizes.txt", cluster_sizes)
