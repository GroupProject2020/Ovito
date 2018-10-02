from ovito.io import import_file
from ovito.data import SimulationCell, DataCollection


# Load input simulation file.
#pipeline = import_file("input/simulation.dump")

################ Snippet code begins here #################
data = DataCollection()
cell = SimulationCell()
data.objects.append(cell)
assert(data.cell is cell)
################ Snippet code ends here #################
