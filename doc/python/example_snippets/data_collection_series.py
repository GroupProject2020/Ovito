from ovito.io import import_file
from ovito.modifiers import CoordinationAnalysisModifier

pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(CoordinationAnalysisModifier(cutoff = 2.9))
data = pipeline.compute()

################# Code snippet begin #####################
print(data.series)
################## Code snippet end ######################


################# Code snippet begin #####################
rdf_series = data.series['coordination-rdf']
print(rdf_series.as_table())
################3# Code snippet end ######################

