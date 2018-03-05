from ovito.io import import_file, export_file
from ovito.modifiers import PythonScriptModifier, CalculateDisplacementsModifier
import numpy

# Load input data and create a data pipeline.
pipeline = import_file("input/simulation.dump")

# Calculate per-particle displacements with respect to initial simulation frame:
pipeline.modifiers.append(CalculateDisplacementsModifier())

# Define the custom modifier function:
def modify(frame, input, output):

    # Access the per-particle displacement magnitudes computed by the 
    # CalculateDisplacementsModifier that precedes this user-defined modifier in the 
    # data pipeline:
    displacement_magnitudes = input.particles['Displacement']

    # Compute MSD:
    msd = numpy.sum(displacement_magnitudes ** 2) / len(displacement_magnitudes)

    # Output MSD value as a global attribute: 
    output.attributes["MSD"] = msd 

# Insert custom modifier into the data pipeline.
pipeline.modifiers.append(PythonScriptModifier(function = modify))

# Export calculated MSD value to a text file and let OVITO's data pipeline do the rest:
export_file(pipeline, "output/msd_data.txt", 
    format = "txt",
    columns = ["Timestep", "MSD"],
    multiple_frames = True)
