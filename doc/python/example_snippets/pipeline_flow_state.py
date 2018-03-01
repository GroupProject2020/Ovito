from ovito.io import import_file
from ovito.data import SimulationCell
pipeline = import_file("input/simulation.dump")

data = pipeline.compute()

# Modifying the SimulationCell object as follows is not permitted, 
# because it is owned by the pipeline:
cell = data.expect(SimulationCell)
cell.pbc = (True, True, True)  # WRONG!!!

# CORRECT: Copy data object first, then modify it: 
cell = data.copy_if_needed(data.expect(SimulationCell))
cell.pbc = (True, True, True)

cell = data.expect(SimulationCell)
cell.vis.line_width = 0.5