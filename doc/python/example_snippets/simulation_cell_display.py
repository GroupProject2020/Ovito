from ovito.io import import_file
from ovito.data import SimulationCell

pipeline = import_file("simulation.dump")
pipeline.add_to_scene()

cell = pipeline.source.expect(SimulationCell)
cell.display.line_width = 1.3
