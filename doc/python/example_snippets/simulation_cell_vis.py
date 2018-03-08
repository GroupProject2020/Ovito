from ovito.io import import_file
from ovito.data import SimulationCell

pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()

cell = pipeline.source.expect(SimulationCell)
cell.vis.line_width = 1.3
cell.vis.rendering_color = (0.0, 0.0, 0.8)