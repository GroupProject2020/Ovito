from ovito.io import import_file, export_file
from ovito.modifiers import (CoordinationAnalysisModifier, FreezePropertyModifier,
                            ExpressionSelectionModifier)

# Load a input simulation sequence. 
pl = import_file("input/simulation.*.dump")

# Add modifier for computing the coordination numbers of particles.
pl.modifiers.append(CoordinationAnalysisModifier(cutoff = 2.9))

# Save the initial coordination numbers from frame 0 under a new name.
modifier = FreezePropertyModifier(source_property = 'Coordination', 
                                  destination_property = 'Coord0',
                                  freeze_at = 0)
pl.modifiers.append(modifier)

# Select all particles whose coordination number has changed since frame 0
# by comapring the dynamically computed coordination numbers with the frozen ones.
pl.modifiers.append(ExpressionSelectionModifier(expression='Coordination != Coord0'))

# Write out number of particles exhibiting a change in coordination number.
export_file(pl, 'output/changes.txt', 'txt', 
            columns = ['Timestep', 'SelectExpression.num_selected'], 
            multiple_frames = True)
