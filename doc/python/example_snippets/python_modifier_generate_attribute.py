from ovito.io import import_file
from ovito.modifiers import CommonNeighborAnalysisModifier

pipeline = import_file("input/simulation.dump")
################# Snippet code begins here ##########################
pipeline.modifiers.append(CommonNeighborAnalysisModifier())
            
def compute_fcc_fraction(frame, data):
    n_fcc = data.attributes['CommonNeighborAnalysis.counts.FCC']
    data.attributes['fcc_fraction'] = n_fcc / data.particles.count

pipeline.modifiers.append(compute_fcc_fraction)
print(pipeline.compute().attributes['fcc_fraction'])
