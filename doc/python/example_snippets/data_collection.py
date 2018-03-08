from ovito.io import import_file
from ovito.data import SimulationCell
from ovito.pipeline import StaticSource

# Load input simulation file.
pipeline = import_file("input/simulation.dump")

# >>>>>>>
data_collection = pipeline.compute()
cell = data_collection.find(SimulationCell)
assert(cell is None or cell in data_collection.objects)
# <<<<<<<

# >>>>>>>
cell = data_collection.expect(SimulationCell)
assert(cell is not None and cell in data_collection.objects)
# <<<<<<<

# >>>>>>>
cell = SimulationCell()
my_data_collection = StaticSource()
my_data_collection.objects.append(cell)
# <<<<<<<

# >>>>>>>
positions = data_collection.particles['Position']
assert(positions in data_collection.objects)
# <<<<<<<

# >>>>>>>
positions = data_collection.particles['Position']
with positions:                # NEVER DO THIS! You may accidentally be manipulating 
    positions[...] += (0,0,2)  # shared data that is owned by the pipeline system!
# <<<<<<<

# >>>>>>>
# First, pass the data object from the collection to the copy_if_needed() method.
# A data copy is made if and only if the collection is not yet the exclusive owner.
positions = data_collection.copy_if_needed( data_collection.particles['Position'] )

# Now it's guaranteed that the data object is not shared by any other collection 
# and it has become safe to modify its contents:
with positions:
    positions[...] += (0,0,2) # Displacing all particles along z-direction.
# <<<<<<<
