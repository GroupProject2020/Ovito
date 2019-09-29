from ovito.io import *
from ovito.data import *
from ovito.modifiers import *
from ovito.pipeline import *
import numpy as np

pipeline = import_file("input/simulation.*.dump")

# Perform Wigner-Seitz analysis:
ws = WignerSeitzAnalysisModifier(
    per_type_occupancies = True,
    affine_mapping = ReferenceConfigurationModifier.AffineMapping.ToReference)
pipeline.modifiers.append(ws)

# Define a modifier function that selects sites of type A=1 which
# are occupied by exactly one atom of type B=2.
def modify(frame, data):

    # Retrieve the two-dimensional array with the site occupancy numbers.
    # Use [...] to cast it to a Numpy array.
    occupancies = data.particles['Occupancy']

    # Get the site types as additional input:
    site_type = data.particles['Particle Type']

    # Calculate total occupancy of every site:
    total_occupancy = np.sum(occupancies, axis=1)

    # Set up a particle selection by creating the Selection property:
    selection = data.particles_.create_property('Selection')

    # Select A-sites occupied by exactly one B-atom (the second entry of the Occupancy
    # array must be 1, and all others 0). Note that the Occupancy array uses 0-based
    # indexing, while atom type IDs are typically 1-based.
    selection[...] = (site_type == 1) & (occupancies[:,1] == 1) & (total_occupancy == 1)

    # Additionally output the total number of antisites as a global attribute:
    data.attributes['Antisite_count'] = np.count_nonzero(selection)

# Insert Python modifier into the data pipeline.
pipeline.modifiers.append(modify)

# Let OVITO do the computation and export the number of identified
# antisites as a function of simulation time to a text file:
export_file(pipeline, "output/antisites.txt", "txt/attr",
    columns = ['Timestep', 'Antisite_count'],
    multiple_frames = True)

# Export the XYZ coordinates of just the antisites by removing all other atoms.
pipeline.modifiers.append(InvertSelectionModifier())
pipeline.modifiers.append(DeleteSelectedModifier())
export_file(pipeline, "output/antisites.xyz", "xyz",
    columns = ['Position.X', 'Position.Y', 'Position.Z'],
    multiple_frames = True)
