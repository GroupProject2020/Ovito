from ovito.io import import_file
from ovito.modifiers import *

pipeline = import_file("input/first_file.dump")
data = pipeline.compute()

################# Code snippet begin #####################
original = data.clone()
data.apply(ExpressionSelectionModifier(expression="Position.Z < 0"))
data.apply(DeleteSelectedModifier())
print("Number of atoms before:", original.particles.count)
print("Number of atoms after:", data.particles.count)
################## Code snippet end ######################


################# Code snippet begin #####################
copy = data.clone()
# Data objects are shared by original and copy:
assert(copy.cell is data.cell)

# In order to modify the SimulationCell in the dataset copy, we must request
# a mutable version of the SimulationCell using the 'cell_' accessor:
copy.cell_.pbc = (False, False, False)

# As a result, the cell object in the second data collection has been replaced
# with a deep copy and the two data collections no longer share the same
# simulation cell object:
assert(copy.cell is not data.cell)
################## Code snippet end ######################
