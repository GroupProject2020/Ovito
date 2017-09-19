from ovito.io import import_file

# Create a new pipeline with a FileSource:
pipeline = import_file('first_file.dump')

# Get the data from the first file:
data1 = pipeline.compute()

# Replace the pipeline's input with a different file by calling FileSource.load():
pipeline.source.load('second_file.dump')
data2 = pipeline.compute()