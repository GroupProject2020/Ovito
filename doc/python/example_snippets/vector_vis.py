from ovito.io import import_file
from ovito.vis import VectorVis
from ovito.modifiers import CalculateDisplacementsModifier

# >>>>>>>>
pipeline = import_file('input/simulation.dump')
pipeline.add_to_scene()
vector_vis = pipeline.source.data.particles.forces.vis
vector_vis.color = (1,0,0)
vector_vis.enabled = True  # This activates the display of arrows
# <<<<<<<<


# >>>>>>>>
modifier = CalculateDisplacementsModifier()
pipeline.modifiers.append(modifier)
modifier.vis.enabled = True  # This activates the display of displacement vectors
modifier.vis.shading = VectorVis.Shading.Flat
# <<<<<<<<


# >>>>>>>>
from ovito.vis import VectorVis
import numpy

# Set up the visual element outside of the modifier function:
vector_vis = VectorVis(
    alignment = VectorVis.Alignment.Center, 
    color = (1.0, 0.0, 0.4))

def my_modifier_function(frame, data):
    vector_data = numpy.random.random_sample(size=(data.particles.count, 3))
    my_prop = data.particles_.create_property('My Property', data=vector_data)
    my_prop.vis = vector_vis  # Attach the visual element to output property
# <<<<<<<<

pipeline.modifiers.append(my_modifier_function)
data = pipeline.compute()
assert(data.particles['My Property'].vis is vector_vis)