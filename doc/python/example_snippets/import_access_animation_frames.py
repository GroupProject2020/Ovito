from ovito.io import import_file

# Import a sequence of files.
pipeline = import_file('simulation.*.dump')

# Loop over all frames of the sequence.
for frame in range(pipeline.source.num_frames):    

    # Evaluating the pipeline at the current animation frame loads the 
    # corresponding input file from the sequence into memory:
    data = pipeline.compute(frame)
    
    # Work with the loaded data...
    print("Particle positions at frame %i:" % frame)
    print(data.particle_properties['Position'][...])
