from ovito.io import import_file

# Load some input data:
pipeline = import_file("input/simulation.dump")

# Define our custom modifier function, which assigns a uniform color
# to all particles, similar to the built-in AssignColorModifier.
def assign_color(frame, data):
    color_property = data.particles_.create_property('Color')
    color_property[:] = (0.2, 0.5, 1.0)

# Insert the user-defined modifier function into the data pipeline.
pipeline.modifiers.append(assign_color)
# Note that appending the Python function to the pipeline is equivalent to
# creating a PythonScriptModifier instance and appending it:
#
#   pipeline.modifiers.append(PythonScriptModifier(function = assign_color))

# Evaluate data pipeline. This will make the system invoke assign_color().
data = pipeline.compute()
print(data.particles.colors[...])