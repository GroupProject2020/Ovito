from ovito.data import *
import math
import numpy as np

def quaternions_to_colors(qs):
    """ Takes a list of quaternions (Nx4 array) and returns a list of
        corresponding RGB colors (Nx3 array) """
 
    if len(qs.shape) != 2:
        raise RuntimeError("qs must be a 2-dimensional array")
 
    if qs.shape[1] != 4:
        raise RuntimeError("qs must be a n x 4 dimensional array")
 
    # Project quaternions into Rodrigues space: rs = (qs.X/qs.W, qs.Y/qs.W, qs.Z/qs.W)
    # Note that the qs.W may be zero for particles for which no lattice orientation
    # could be computed by the PTM modifier.
    rs = np.zeros_like(qs[:,:3])
    np.divide(qs[:,0], qs[:,3], out=rs[:,0], where=qs[:,3] != 0)
    np.divide(qs[:,1], qs[:,3], out=rs[:,1], where=qs[:,3] != 0)
    np.divide(qs[:,2], qs[:,3], out=rs[:,2], where=qs[:,3] != 0)

    # Compute vector lengths rr = norm(rs)
    rr = np.linalg.norm(rs, axis=1)
    rr = np.maximum(rr, 1e-9) # hack
 
    # Normalize Rodrigues vectors.
    rs[:,0] /= rr
    rs[:,1] /= rr
    rs[:,2] /= rr
 
    theta = 2 * np.arctan(rr)
    rs[:,0] *= theta
    rs[:,1] *= theta
    rs[:,2] *= theta
    
    # Normalize values.
    rs += math.radians(62.8)
    rs[:,0] /= 2*math.radians(62.8)
    rs[:,1] /= 2*math.radians(62.8)
    rs[:,2] /= 2*math.radians(62.8)
    
    return rs
    
def modify(frame, input, output):
    """ The custom modifier function """
    
    # The input:
    orientation_property = input.particle_properties.orientation.array
    
    # The output:
    color_property = output.create_particle_property(ParticleProperty.Type.Color)
    color_property.marray[:] = quaternions_to_colors(orientation_property)

# The following is for automated testing only and not shown in the documentation:
from ovito.io import import_file
from ovito.modifiers import PythonScriptModifier, PolyhedralTemplateMatchingModifier
node = import_file("simulation.dump")
node.modifiers.append(PolyhedralTemplateMatchingModifier(output_orientation = True))
node.modifiers.append(PythonScriptModifier(function = modify))
node.compute()