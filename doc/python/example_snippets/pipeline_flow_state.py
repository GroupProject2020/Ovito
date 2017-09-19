from ovito.io import import_file
from ovito.data import SimulationCell
pipeline = import_file("simulation.dump")

data = pipeline.compute()

# WRONG: Modifying the SimulationCell data object is not permitted, 
# because it is owned by the pipeline:
cell = data.expect(SimulationCell)
cell.pbc = (True, True, True)

# CORRECT: copy first, then modify 
cell = data.copy_if_needed(data.expect(SimulationCell))
cell.pbc = (True, True, True)

# No need to copy the DataObject when modifying only its Display object
cell = data.expect(SimulationCell)
cell.display.line_width = 0.5