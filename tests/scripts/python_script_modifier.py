from ovito.io import import_file
from ovito.modifiers import PythonScriptModifier
from ovito.data import *
import numpy

pipeline = import_file("../files/CFG/shear.void.120.cfg")
pipeline.add_to_scene()

def compute_coordination(pindex, finder):
    return sum(1 for _ in finder.find(pindex))

def mymodify(frame, data):
    yield "Hello world"
    print("mymodify()")
    data.particles.vis.radius = 0.5
    color_property = data.particles_.create_property("Color")
    color_property[:] = (0,0.5,0)
    my_property = data.particles_.create_property("MyCoordination", dtype=int, components=1)
    finder = CutoffNeighborFinder(3.5, data)
    for index in range(data.particles.count):
        if index % 100 == 0: yield index/data.particles.count
        my_property[index] = compute_coordination(index, finder)

modifier = PythonScriptModifier(function = mymodify)
pipeline.modifiers.append(modifier)
data = pipeline.compute()
assert(numpy.all(data.particles['Color'][0] == numpy.array([0,0.5,0])))
assert(data.particles["MyCoordination"][0] > 0)

# Testing backward compatibility with OVITO 2.9.0:
def mymodify_old(frame, input, output):
    yield "Hello world"
    print("mymodify_old()")
    color_property = output.create_particle_property(int(Particles.Type.Color))
    color_property.marray[:] = (0,0.6,0)
    my_property = output.create_user_particle_property("MyCoordination", "int")
    finder = CutoffNeighborFinder(3.5, input)
    for index in range(input.number_of_particles):
        if index % 100 == 0: yield index/input.number_of_particles
        my_property.marray[index] = compute_coordination(index, finder)

modifier.function = mymodify_old
data = pipeline.compute()
assert(numpy.all(data.particles['Color'][0] == numpy.array([0,0.6,0])))
assert(data.particles["MyCoordination"][0] > 0)
