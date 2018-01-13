from ovito.io import import_file
from ovito.data import SimulationCell

pipeline = import_file("input/simulation.dump")
cell = pipeline.source.expect(SimulationCell)

# Print cell matrix to the console. [...] is for casting to Numpy. 
print(cell[...])

import numpy.linalg

# Compute simulation cell volume by taking the determinant of the
# left 3x3 submatrix of the cell matrix:
vol = abs(numpy.linalg.det(cell[0:3,0:3]))

# The SimulationCell.volume property computes the same value.
assert numpy.isclose(cell.volume, vol)

# Make cell twice as large along the Y direction by scaling the second cell vector: 
with cell:
    cell[:,1] *= 2.0

# Change display color of simulation cell to red:
cell.display.rendering_color = (1.0, 0.0, 0.0)
