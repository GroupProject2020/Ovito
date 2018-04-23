from ovito.io import import_file
from ovito.data import SimulationCell, DataCollection


# Load input simulation file.
pipeline = import_file("input/simulation.dump")

# >>>>>>>
data = pipeline.compute()
cell = data.find(SimulationCell)
assert(cell is None or cell in data.objects)
# <<<<<<<

# >>>>>>>
cell = data.expect(SimulationCell)
assert(cell is not None and cell in data.objects)
# <<<<<<<

# >>>>>>>
cell = SimulationCell()
mydata = DataCollection()
mydata.objects.append(cell)
# <<<<<<<

# >>>>>>>
positions = data.particles['Position']
assert(positions in data.objects)
# <<<<<<<
# >>>>>>>
data = pipeline.compute()
positions = data.particles['Position']
with positions:                # NEVER DO THIS! You may accidentally be manipulating 
    positions[...] += (0,0,2)  # shared data that is owned by the pipeline system!
# <<<<<<<

# >>>>>>>
# First, pass the data object from the collection to the copy_if_needed() method.
# A data copy is made if and only if the collection is not yet the exclusive owner.
positions = data.copy_if_needed( data.particles['Position'] )

# Now it's guaranteed that the data object is not shared by any other collection 
# and it has become safe to modify its contents:
with positions:
    positions[...] += (0,0,2) # Displacing all particles along z-direction.
# <<<<<<<
