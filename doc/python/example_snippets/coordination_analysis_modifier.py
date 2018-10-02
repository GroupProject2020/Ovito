from ovito.io import import_file
from ovito.modifiers import CoordinationAnalysisModifier
import numpy

# Load a particle dataset, apply the modifier, and evaluate pipeline.
pipeline = import_file("input/simulation.dump")
modifier = CoordinationAnalysisModifier(cutoff = 5.0, number_of_bins = 200)
pipeline.modifiers.append(modifier)
data = pipeline.compute()

# Export the computed RDF data to a text file.
numpy.savetxt("output/rdf.txt", data.series['coordination-rdf'].as_table())
