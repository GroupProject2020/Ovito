from ovito.io import import_file
from ovito.modifiers import ExpressionSelectionModifier
import numpy
pipeline = import_file("../../../examples/data/NanocrystallinePd.dump.gz")

# Select all atoms with potential energy above -3.6 eV: 
pipeline.modifiers.append(ExpressionSelectionModifier(expression = 'PotentialEnergy > -3.6'))
data = pipeline.compute()
# Demonstrating two ways to get the number of selected atoms:
print(data.attributes['SelectExpression.num_selected'])
print(numpy.count_nonzero(data.particles.selection))