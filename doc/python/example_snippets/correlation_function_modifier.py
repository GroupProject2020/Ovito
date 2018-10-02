from ovito.modifiers import CorrelationFunctionModifier
from ovito.io import import_file, export_file

pipeline = import_file("input/simulation.dump")

mod = CorrelationFunctionModifier(property1='Position.X', property2='peatom')
pipeline.modifiers.append(mod)
data = pipeline.compute()

# Export RDF and correlation functions to text files:
C = data.series['correlation-real-space']
rdf = data.series['correlation-real-space-rdf']
export_file(C, 'output/real_correlation_function.txt', 'txt/series')
export_file(rdf, 'output/rdf.txt', 'txt/series')

# Compute normalized correlation function (by co-variance):
C_norm = C.as_table()
mean1 = data.attributes['CorrelationFunction.mean1']
mean2 = data.attributes['CorrelationFunction.mean2']
covariance = data.attributes['CorrelationFunction.covariance']
C_norm[:,1] = (C_norm[:,1] - mean1*mean2) / (covariance - mean1*mean2)
import numpy
numpy.savetxt('output/normalized_real_correlation_function.txt', C_norm)