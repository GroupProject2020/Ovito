from ovito.io import import_file
from ovito.modifiers import PythonScriptModifier

# Load input data and create a data pipeline.
pipeline = import_file("input/simulation.dump")

# >>>>>>>>> snippet start here >>>>>>>>>>>>>>>
from ovito.data import CutoffNeighborFinder

# Control parameter:
overlap_distance = 2.5

# The user-defined modifier function:
def modify(frame, input, output):

    # Show this text in the status bar while the modifier function executes:
    yield "Selecting overlapping particles"

    # Create 'Selection' output particle property
    selection = output.particles.create_property('Selection')
    
    # Prepare neighbor finder
    finder = CutoffNeighborFinder(overlap_distance, input)
    
    # Request write access to the output property array
    with selection:
    
        # Iterate over all particles
        for index in range(len(selection)):
        
            # Progress bar update
            yield (index / len(selection))
            
            # Iterate over all closeby particles around the current center particle
            for neigh in finder.find(index):
                
                # Once we find a neighbor which hasn't been marked yet, 
                # mark the current center particle. This test is to ensure that we
                # always select only one of the particles in a close pair.
                if selection[neigh.index] == 0:
                    selection[index] = 1
                    break
# <<<<<<<<<<<<< snippet ends here <<<<<<<<<<<<<

pipeline.modifiers.append(PythonScriptModifier(function = modify))
data = pipeline.compute()
assert('Selection' in data.particles.keys())
