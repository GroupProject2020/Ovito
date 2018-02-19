import ovito
from ovito.io import *
from ovito.modifiers import *
import numpy as np
import os

pipeline = import_file("../../files/NetCDF/sheared_aSi.nc")

modifier = WignerSeitzAnalysisModifier()
pipeline.modifiers.append(modifier)
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

print("  keep_current_config: {}".format(modifier.keep_current_config))
modifier.keep_current_config = False

data = pipeline.compute()

print("Output:")
print("  vacancy_count= {}".format(data.attributes['WignerSeitz.vacancy_count']))
print("  interstitial_count= {}".format(data.attributes['WignerSeitz.interstitial_count']))
print(data.particle_properties["Occupancy"].array)

assert(data.attributes['WignerSeitz.vacancy_count'] == 970)

pipeline.source.load('../../files/CFG/shear.void.120.cfg')
print(len(pipeline.source.particle_properties['Particle Type'].types))
modifier.per_type_occupancies = True
data = pipeline.compute()
print("number_of_particles: %i" % data.number_of_particles)
print("occupancy.shape=%s" % str(data.particle_properties["Occupancy"].shape))
print(data.particle_properties["Occupancy"][...])

export_file(pipeline, "_output.dump", "lammps/dump", columns = [ "Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z", "Occupancy","Occupancy.1"])
os.remove("_output.dump")