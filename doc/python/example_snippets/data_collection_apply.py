import numpy

################# Code snippet begin #####################
from ovito.io import import_file
from ovito.modifiers import *

data = import_file("input/simulation.dump").compute()
data.apply(CoordinationAnalysisModifier(cutoff=2.9))
data.apply(ExpressionSelectionModifier(expression="Coordination<9"))
data.apply(DeleteSelectedModifier())
################## Code snippet end ######################


################# Code snippet begin #####################
pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(CoordinationAnalysisModifier(cutoff=2.9))
pipeline.modifiers.append(ExpressionSelectionModifier(expression="Coordination<9"))
pipeline.modifiers.append(DeleteSelectedModifier())
data = pipeline.compute()
################## Code snippet end ######################


################# Code snippet begin #####################
# A user-defined modifier function that calls the built-in ColorCodingModifier
# as a sub-routine to assign a color to each atom based on some property
# created within the function itself:
def modify(frame, data):
    data.particles_.create_property('idx', data=numpy.arange(data.particles.count))
    data.apply(ColorCodingModifier(property='idx'))

# Set up a data pipeline that uses the user-defined modifier function:
pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(modify)
data = pipeline.compute()
################## Code snippet end ######################
