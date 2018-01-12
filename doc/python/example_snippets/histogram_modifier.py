from ovito.modifiers import HistogramModifier
from ovito.io import import_file

pipeline = import_file("input/simulation.dump")

modifier = HistogramModifier(bin_count=100, property='peatom')
pipeline.modifiers.append(modifier)
pipeline.compute()

import numpy
numpy.savetxt("output/histogram.txt", modifier.histogram)