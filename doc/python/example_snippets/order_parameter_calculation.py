from ovito.io import import_file


# Load input data and create a data pipeline.
pipeline = import_file("input/simulation.dump")


################## Code snippet begins here #####################
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
################## Code snippet ends here #####################


################## Code snippet begins here ###################
def modify(frame, data):

    # Show a status text in the status bar:
    yield 'Calculating order parameters'

    # Create output particle property.
    order_params = data.particles_.create_property(
        'Order Parameter', dtype=float, components=1)
    
    # Prepare neighbor lists.
    neigh_finder = NearestNeighborFinder(num_neighbors, data)
    
    # Request write access to the output property array.
    with order_params:

        # Loop over all particles.
        for i in range(data.particles.count):
            
            # Update progress indicator in the status bar
            yield (i/data.particles.count)
            
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

            # Store result in output array.
            order_params[i] = oparam / num_neighbors		
################## Code snippet ends here #####################

pipeline.modifiers.append(modify)
pipeline.compute()
