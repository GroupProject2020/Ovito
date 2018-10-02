from ovito.io import import_file

# Import a sequence of files.
pipeline = import_file('input/simulation.*.dump')

# Loop over all frames of the sequence.
for frame_index in range(pipeline.source.num_frames):    

    # Calling FileSource.compute() loads the requested frame
    # from the sequence into memory and returns the data as a new
    # DataCollection:
    data = pipeline.source.compute(frame_index)

    # The source path and the index of the current frame
    # are attached as attributes to the data collection:
    print("Frame source:", data.attributes['SourceFile'])
    print("Frame index:", data.attributes['SourceFrame'])
    
    # Accessing the loaded frame data, e.g the particle positions:
    print(data.particles.positions[...])