from ovito.io import import_file
from ovito.modifiers import PythonScriptModifier

# Load input data and create a data pipeline.
pipeline = import_file("simulation.dump")

from ovito.data import NearestNeighborFinder
import numpy as np

# The lattice constant of the FCC crystal:
lattice_parameter = 3.6 

# The list of <110> ideal neighbor vectors of the reference lattice (FCC):
reference_vectors = np.asarray([
    (0.5, 0.5, 0.0),
    (-0.5, 0.5, 0.0),
    (0.5, -0.5, 0.0),
    (-0.5, -0.5, 0.0),
    (0.0, 0.5, 0.5),
    (0.0, -0.5, 0.5),
    (0.0, 0.5, -0.5),
    (0.0, -0.5, -0.5),
    (0.5, 0.0, 0.5),
    (-0.5, 0.0, 0.5),
    (0.5, 0.0, -0.5),
    (-0.5, 0.0, -0.5)
])
# Rescale ideal lattice vectors with lattice constant.
reference_vectors *= lattice_parameter

# The number of neighbors to take into account per atom:
num_neighbors = len(reference_vectors)

def modify(frame, input, output):

    # Show a status text in the status bar:
    yield "Calculating order parameters"

    # Create output property.
    order_params = output.particle_properties.create(
        "Order Parameter", dtype=float, components=1)
    
    # Prepare neighbor lists.
    neigh_finder = NearestNeighborFinder(num_neighbors, input)
    
    # Request write access to the output property array.
    with order_params:

        # Loop over all particles
        for i in range(len(order_params)):
            
            # Update progress indicator in the status bar
            yield (i/len(order_params))
            
            # Stores the order parameter of the current atom
            oparam = 0.0	
            
            # Loop over neighbors of current atom.
            for neigh in neigh_finder.find(i):
                
                # Compute squared deviation of neighbor vector from every 
                # reference vector.
                squared_deviations = np.linalg.norm(
                    reference_vectors - neigh.delta, axis=1) ** 2
                
                # Sum up the contribution from the best-matching vector.
                oparam += np.min(squared_deviations)

            # Store result in output particle property.
            order_params[i] = oparam / num_neighbors		

pipeline.modifiers.append(PythonScriptModifier(function = modify))
pipeline.compute()
