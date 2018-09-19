from ovito.io import import_file

pipeline = import_file("input/simulation.dump")
data = pipeline.compute()
cell = data.cell
# Print cell matrix to the console. [...] is for casting to Numpy array. 
print(cell[...])

import numpy.linalg

# Compute simulation box volume by taking the determinant of the
# left 3x3 submatrix of the cell matrix:
vol = abs(numpy.linalg.det(cell[0:3,0:3]))

# The SimulationCell.volume property yields the same value.
assert numpy.isclose(cell.volume, vol)

# Make cell twice as large along the Y direction by scaling the second cell vector: 
with data.cell_:
    data.cell_[:,1] *= 2.0

# Change display color of simulation cell to red:
cell.vis.rendering_color = (1.0, 0.0, 0.0)
