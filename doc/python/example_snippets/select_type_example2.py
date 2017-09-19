from ovito.io import import_file
from ovito.modifiers import SelectTypeModifier, CommonNeighborAnalysisModifier

pipeline = import_file("simulation.dump")

# >>>>>>>>>>> begin snippet >>>>>>>>>>>>>>
# Let the CNA modifier identify the structural type of each particle:
pipeline.modifiers.append(CommonNeighborAnalysisModifier())
# Select all FCC and HCP particles:
modifier = SelectTypeModifier(property = 'Structure Type')
modifier.types = { CommonNeighborAnalysisModifier.Type.FCC,
                   CommonNeighborAnalysisModifier.Type.HCP }
pipeline.modifiers.append(modifier)
# >>>>>>>>>>>> end snippet >>>>>>>>>>>>>>>

data = pipeline.compute()
assert(data.attributes['SelectType.num_selected'] > 0)