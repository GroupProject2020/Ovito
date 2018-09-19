from ovito.io import *
from ovito.modifiers import *
import numpy as np

pipeline = import_file("../../files/LAMMPS/frank_read.dump.gz")
pipeline.modifiers.append(CentroSymmetryModifier(num_neighbors = 12))

modifier = CorrelationFunctionModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  property1: {}".format(modifier.property1))
print("  property2: {}".format(modifier.property2))
print("  grid_spacing: {}".format(modifier.grid_spacing))
print("  apply_window: {}".format(modifier.apply_window))
print("  direct_summation: {}".format(modifier.direct_summation))
print("  neighbor_cutoff: {}".format(modifier.neighbor_cutoff))
print("  neighbor_bins: {}".format(modifier.neighbor_bins))
modifier.neighbor_cutoff = 4.0
modifier.neighbor_bins = 100
modifier.property1 = 'Centrosymmetry'
modifier.property2 = 'Position.X'
data = pipeline.compute()

print("Real-space correlation:")
table = data.series['correlation-real-space'].as_table()
print(table) 
assert(table.ndim == 2 and table.shape[1] == 2)

print("Real-space RDF:")
table = data.series['correlation-real-space-rdf'].as_table()
print(table) 
assert(table.ndim == 2 and table.shape[1] == 2)

print("Reciprocal-space correlation:")
table = data.series['correlation-reciprocal-space'].as_table()
print(table) 
assert(table.ndim == 2 and table.shape[1] == 2)
