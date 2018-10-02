from ovito.io import import_file
from ovito.modifiers import ConstructSurfaceModifier

pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(ConstructSurfaceModifier())
data = pipeline.compute()

################# Code snippet begin #####################
print(data.surfaces)
################## Code snippet end ######################


################# Code snippet begin #####################
surface = data.surfaces['surface']
print(surface.get_vertices())
################3# Code snippet end ######################

