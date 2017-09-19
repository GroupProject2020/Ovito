from ovito.io import import_file, export_file
from ovito.modifiers import CombineParticleSetsModifier

# Load a first set of particles.
pipeline = import_file('first_file.dump')

# Insert the particles from a second file into the dataset. 
modifier = CombineParticleSetsModifier()
modifier.source.load('second_file.dump')
pipeline.modifiers.append(modifier)

# Export combined dataset to a new file.
export_file(pipeline, 'output.dump', 'lammps/dump',
            columns = ['Position.X', 'Position.Y', 'Position.Z'])
