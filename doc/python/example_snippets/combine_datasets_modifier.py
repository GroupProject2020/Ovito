from ovito.io import import_file, export_file
from ovito.modifiers import CombineDatasetsModifier

# Load a first set of particles.
pipeline = import_file('input/first_file.dump')

# Insert the particles from a second file into the dataset. 
modifier = CombineDatasetsModifier()
modifier.source.load('input/second_file.dump')
pipeline.modifiers.append(modifier)

# Export combined dataset to a new file.
export_file(pipeline, 'output/combined.dump', 'lammps/dump',
            columns = ['Position.X', 'Position.Y', 'Position.Z'])
