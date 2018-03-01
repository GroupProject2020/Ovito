from ovito.io import import_file

# This creates a Pipeline with an attached FileSource.
pipeline = import_file('input/simulation.dump')

# Retrieve the data for the first frame from the FileSource.
data = pipeline.source.compute(0)
print(data.particles['Position'][...])

# Accessing data that is currently cached in the FileSource object itself:
print(pipeline.source.particles['Position'][...])