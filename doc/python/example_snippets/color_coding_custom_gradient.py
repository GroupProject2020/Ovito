from ovito.io import import_file
from ovito.modifiers import ColorCodingModifier
import numpy as np

pipeline = import_file("input/simulation.dump")


################# Code snippet begins here #####################
color_table = [
    (1.0, 0.0, 0.0),  # red
    (1.0, 1.0, 0.0),  # yellow
    (1.0, 1.0, 1.0)   # white
]
modifier = ColorCodingModifier(
    property = 'Position.X',
    gradient = ColorCodingModifier.Gradient(color_table)
)
################# Code snippet ends here #####################

# Verify that the color coding modifier has assigned the colors from the custom color gradient correctly.
pipeline.modifiers.append(modifier)
data = pipeline.compute()
min_index = np.argmin(data.particles["Position"][:,0])
max_index = np.argmax(data.particles["Position"][:,0])
print(data.particles.colors[min_index])
print(data.particles.colors[max_index])
assert(np.allclose(data.particles.colors[min_index], color_table[0]))
assert(np.allclose(data.particles.colors[max_index], color_table[-1]))