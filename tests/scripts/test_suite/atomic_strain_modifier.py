from ovito.io import *
from ovito.modifiers import AffineTransformationModifier, AtomicStrainModifier
from ovito.pipeline import ReferenceConfigurationModifier
import numpy as np

pipeline = import_file("../../files/POSCAR/Ti_n1_PBE.n54_G7_V15.000.poscar.000")

# Apply some strain to the atoms.
deform = AffineTransformationModifier(
    transformation = [[1,0.1,0,0],[0,1,0,0.8],[0,0,1,0]],
    transform_box = True
)
pipeline.modifiers.append(deform)

# Calculate the atomic strain.
modifier = AtomicStrainModifier()
pipeline.modifiers.append(modifier)
print("Loading reference configuration")
modifier.reference.load("../../files/POSCAR/Ti_n1_PBE.n54_G7_V15.000.poscar.000")

print("Parameter defaults:")

# Backward compatibility with OVITO 2.9.0:
print("  assume_unwrapped_coordinates: {}".format(modifier.assume_unwrapped_coordinates))
modifier.assume_unwrapped_coordinates = False

# Backward compatibility with OVITO 2.9.0:
print("  eliminate_cell_deformation: {}".format(modifier.eliminate_cell_deformation))
modifier.eliminate_cell_deformation = False

print("  cutoff: {}".format(modifier.cutoff))
modifier.cutoff = 2.8

print("  frame_offset: {}".format(modifier.frame_offset))
modifier.frame_offset = 0

print("  output_deformation_gradients: {}".format(modifier.output_deformation_gradients))
modifier.output_deformation_gradients = True

print("  output_nonaffine_squared_displacements: {}".format(modifier.output_nonaffine_squared_displacements))
modifier.output_nonaffine_squared_displacements = True

print("  output_strain_tensors: {}".format(modifier.output_strain_tensors))
modifier.output_strain_tensors = True

print("  reference_frame: {}".format(modifier.reference_frame))
modifier.reference_frame = 0

print("  select_invalid_particles: {}".format(modifier.select_invalid_particles))
modifier.select_invalid_particles = True

print("  use_frame_offset: {}".format(modifier.use_frame_offset))
modifier.use_frame_offset = True

print("  minimum_image_convention: {}".format(modifier.minimum_image_convention))
modifier.minimum_image_convention = True

print("  affine_mapping: {}".format(modifier.affine_mapping))
modifier.affine_mapping = ReferenceConfigurationModifier.AffineMapping.ToReference
modifier.affine_mapping = ReferenceConfigurationModifier.AffineMapping.ToCurrent
modifier.affine_mapping = ReferenceConfigurationModifier.AffineMapping.Off

data = pipeline.compute()

print("Output:")
print("  invalid_particle_count={}".format(data.attributes["AtomicStrain.invalid_particle_count"]))
print(data.particles['Shear Strain'][...])
print(data.particles['Deformation Gradient'][...])
print(data.particles['Strain Tensor'][...])
print(data.particles['Selection'][...])

assert(np.allclose(data.particles["Shear Strain"], 0.05008306))

pipeline.add_to_scene()

deform.transformation = [[1,2.4,0,0],[0,1,0,0],[0,0,1,0]]
data = pipeline.compute()
assert(np.allclose(data.particles["Shear Strain"], 2.05056))