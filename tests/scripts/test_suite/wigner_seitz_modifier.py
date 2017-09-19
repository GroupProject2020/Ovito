import ovito
from ovito.io import *
from ovito.modifiers import *
import numpy as np
import os

node = import_file("../../files/NetCDF/sheared_aSi.nc")

modifier = WignerSeitzAnalysisModifier()
node.modifiers.append(modifier)
modifier.reference.load("../../files/NetCDF/sheared_aSi.nc")

ovito.dataset.anim.current_frame = 4

print("Parameter defaults:")

print("  eliminate_cell_deformation: {}".format(modifier.eliminate_cell_deformation))
modifier.eliminate_cell_deformation = True

print("  frame_offset: {}".format(modifier.frame_offset))
modifier.frame_offset = 0

print("  reference_frame: {}".format(modifier.reference_frame))
modifier.reference_frame = 0

print("  use_frame_offset: {}".format(modifier.use_frame_offset))
modifier.use_frame_offset = False

print("  per_type_occupancies: {}".format(modifier.per_type_occupancies))
modifier.per_type_occupancies = False

node.compute()

print("Output:")
print("  vacancy_count= {}".format(node.output.attributes['WignerSeitz.vacancy_count']))
print("  interstitial_count= {}".format(node.output.attributes['WignerSeitz.interstitial_count']))
print(node.output.particle_properties["Occupancy"].array)

assert(node.output.attributes['WignerSeitz.vacancy_count'] == 970)

node.source.load('../../files/CFG/shear.void.120.cfg')
print(len(node.source.particle_properties.particle_type.type_list))
modifier.per_type_occupancies = True
node.compute()
print("number_of_particles: %i" % node.output.number_of_particles)
print("occupancy.shape=%s" % str(node.output.particle_properties["Occupancy"].array.shape))
#assert(node.output["Occupancy"].array.shape == (node.output.number_of_particles, 3))
print(node.output.particle_properties["Occupancy"].array)

export_file(node, "_output.dump", "lammps/dump", columns = [ "Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z", "Occupancy","Occupancy.1"])
os.remove("_output.dump")