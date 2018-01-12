from ovito.io import import_file

# This creates a pipeline with a FileSource.
pipeline = import_file('input/simulation.dump')

# Access the data cached in the FileSource.
print(pipeline.source.particle_properties['Position'][...])

