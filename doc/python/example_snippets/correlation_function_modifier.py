from ovito.modifiers import CorrelationFunctionModifier
from ovito.io import import_file

pipeline = import_file("input/simulation.dump")

mod = CorrelationFunctionModifier(property1='Position.X', property2='peatom')
pipeline.modifiers.append(mod)
pipeline.compute()

# Export RDF and correlation functions to text files:
import numpy
C = mod.get_real_space_function()
rdf = mod.get_rdf()
numpy.savetxt("output/real_correlation_function.txt", C)
numpy.savetxt("output/rdf.txt", rdf)

# Compute normalized correlation function (by co-variance):
C_norm = numpy.copy(C)
C[:,1] = (C[:,1] - mod.mean1*mod.mean2) / (mod.covariance - mod.mean1*mod.mean2)
numpy.savetxt("output/normalized_real_correlation_function.txt", C_norm)