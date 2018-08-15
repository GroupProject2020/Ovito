from ovito.io import import_file
from ovito.modifiers import CoordinationAnalysisModifier
import numpy

# Load a simulation trajectory consisting of several frames:
pipeline = import_file("input/simulation.dump")

# Insert the modifier into the pipeline:
modifier = CoordinationAnalysisModifier(cutoff = 5.0, number_of_bins = 200)
pipeline.modifiers.append(modifier)

# Initialize array for accumulated RDF histogram to zero:
total_rdf = numpy.zeros((modifier.number_of_bins, 2))

# Iterate over all frames of the sequence.
for frame in range(pipeline.source.num_frames):
    # Evaluate pipeline to let the modifier compute the RDF of the current frame:
    pipeline.compute(frame)
    # Accumulate RDF histograms:
    total_rdf += modifier.rdf

# Averaging:
total_rdf /= pipeline.source.num_frames

# Export the average RDF to a text file:
numpy.savetxt("output/rdf.txt", total_rdf)
