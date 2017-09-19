from ovito.io import import_file
from ovito.modifiers import CommonNeighborAnalysisModifier

pipeline = import_file("simulation.dump")

pipeline.modifiers.append(CommonNeighborAnalysisModifier())
data = pipeline.compute()
print("Number of FCC atoms: %i" % data.attributes['CommonNeighborAnalysis.counts.FCC'])
