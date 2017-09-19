from ovito.io import import_file
from ovito.data import SimulationCell

# Load input simulation file.
pipeline = import_file("simulation.dump")

data = pipeline.compute()
cell = data.find(SimulationCell)
assert(cell is None or cell in data.objects)

cell = data.expect(SimulationCell)
assert(cell is not None and cell in data.objects)

pos_property = data.particle_properties['Position']
assert(pos_property in data.objects)