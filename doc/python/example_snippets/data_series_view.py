from ovito.io import import_file
from ovito.data import DataSeries, DataCollection
import ovito.pipeline
import numpy
pipeline = import_file("input/simulation.dump")


# snippet begin >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
data = pipeline.compute()
rdf = data.series['coordination/rdf']

# snippet end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

