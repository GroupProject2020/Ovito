from ovito.io import import_file
from ovito.modifiers import SelectTypeModifier, CommonNeighborAnalysisModifier

pipeline = import_file("simulation.dump")

# Insert a CNA modifier to determine the structural type of each atom:
pipeline.modifiers.append(CommonNeighborAnalysisModifier())

# Apply the SelectTypeModifier to select all atoms of FCC and HCP type:
pipeline.modifiers.append(SelectTypeModifier(
    operate_on = "particles",
    property = "Structure Type",
    types = { CommonNeighborAnalysisModifier.Type.FCC,
              CommonNeighborAnalysisModifier.Type.HCP }
))

# The SelectTypeModifier reports the number of selected elements as an attribute:
data = pipeline.compute()
print("Number of FCC/HCP atoms: %i" % data.attributes['SelectType.num_selected'])