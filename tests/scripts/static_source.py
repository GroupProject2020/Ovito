from ovito.pipeline import Pipeline, StaticSource
from ovito.data import DataCollection, SimulationCell

# Create a data collection:
data = DataCollection()
data.objects.append(SimulationCell())

# Create the static pipeline source:
source = StaticSource(data = data)

# Create a pipeline and wire the static source to it.
pipeline = Pipeline(source = source)
output_data = pipeline.compute()
assert(output_data.cell is not None)

# The static source doesn't pass the original data to the pipeline: 
assert(output_data is not data)
assert(output_data.cell is not data.cell)