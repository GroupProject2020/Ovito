from __future__ import division
from ovito.io import *
from ovito.modifiers import *
import numpy as np

pipeline = import_file("../files/CFG/shear.void.120.cfg")

modifier = CoordinationAnalysisModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  cutoff: {}".format(modifier.cutoff))
print("  number_of_bins: {}".format(modifier.number_of_bins))
print("  partial: {}".format(modifier.partial))
modifier.cutoff = 3.0
modifier.number_of_bins = 400

data = pipeline.compute()

print("Output:")
print(data.particles['Coordination'][...])
print("RDF:")
total_rdf = data.series["coordination-rdf"]
assert(total_rdf.count == modifier.number_of_bins)
assert(len(total_rdf.y) == modifier.number_of_bins)

# Compute partial RDFs
modifier.partial = True
data = pipeline.compute()
type_prop = data.particles['Particle Type']
ntypes = len(type_prop.types)
counts = [np.count_nonzero(type_prop == t) for t in range(1,ntypes+1)]
print("Number of particle types:", ntypes)
print("Number of particles of each type:", counts)
partial_rdfs = data.series["coordination-rdf"].y
print("partial_rdfs.shape:", partial_rdfs.shape)
print("partial_rdfs:")
print(partial_rdfs[...])

assert(partial_rdfs.shape == (modifier.number_of_bins, ntypes*(ntypes+1)//2))
assert(data.series["coordination-rdf"].as_table().shape == (modifier.number_of_bins, 1+ntypes*(ntypes+1)//2))

# Check if partial RDFs correctly sum up to the total RDF.
my_total_rdf = np.zeros_like(total_rdf.y)
idx = 0
for i in range(ntypes):
    for j in range(i,ntypes):
        factor = counts[i] * counts[j] / len(type_prop) / len(type_prop)
        if i != j: factor *= 2
        my_total_rdf += factor * partial_rdfs[:,idx]
        idx += 1
print("my_total_rdf:")
print(my_total_rdf)
print("total_rdf.y:")
print(total_rdf.y[...])
assert(np.allclose(my_total_rdf, total_rdf.y))
