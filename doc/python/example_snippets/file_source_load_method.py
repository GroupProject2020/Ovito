from ovito.io import import_file

# Create a new pipeline with a FileSource:
pipeline = import_file('input/first_file.dump')

# Get the data from the first file:
data1 = pipeline.compute()

# Use FileSource.load() method to replace the pipeline's input with a different file 
pipeline.source.load('input/second_file.dump')

# Now the pipeline gets its input data from the new file:
data2 = pipeline.compute()