from ovito.io import import_file
from ovito.modifiers import PythonScriptModifier

# Load some input data:
pipeline = import_file("simulation.dump")

# Define our custom modifier function, which assigns a uniform color 
# to all particles, similar to the built-in AssignColorModifier. 
def assign_color(frame, input, output):
    color_property = output.particle_properties.create('Color')
    with color_property.modify() as colors:
        colors[:] = (0.2, 0.5, 1.0)

# Insert custom modifier into the data pipeline.
pipeline.modifiers.append(PythonScriptModifier(function = assign_color))

# Evaluate data pipeline. This will result in a call to assign_color() from above.
data = pipeline.compute()
print(data.particle_properties['Color'][...])