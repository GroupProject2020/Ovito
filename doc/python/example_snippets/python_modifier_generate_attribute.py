from ovito.io import import_file
from ovito.modifiers import PythonScriptModifier, CommonNeighborAnalysisModifier

pipeline = import_file("input/simulation.dump")

pipeline.modifiers.append(CommonNeighborAnalysisModifier())
            
def compute_fcc_fraction(frame, input, output):
    n_fcc = input.attributes['CommonNeighborAnalysis.counts.FCC']
    output.attributes['fcc_fraction'] = n_fcc / input.particles.count


pipeline.modifiers.append(PythonScriptModifier(function = compute_fcc_fraction))
print(pipeline.compute().attributes['fcc_fraction'])
