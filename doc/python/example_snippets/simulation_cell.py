#================ Begin snippet =========================
from ovito.io import import_file

pipeline = import_file("input/simulation.dump")
data = pipeline.compute()
# Print cell matrix to the console. [...] is for casting to Numpy array.
print(data.cell[...])
#================= End snippet ==========================


import numpy.linalg

#================ Begin snippet =========================
# Compute simulation box volume by taking the determinant of the
# left 3x3 submatrix of the cell matrix:
vol = abs(numpy.linalg.det(data.cell[0:3,0:3]))

# The SimulationCell.volume property yields the same value.
assert numpy.isclose(data.cell.volume, vol)
#================= End snippet ==========================

#================ Begin snippet =========================
# Make cell twice as large along the Y direction by scaling the second cell vector:
data.cell_[:,1] *= 2.0
#================= End snippet ==========================

#================ Begin snippet =========================
# Change display color of simulation cell to red:
data.cell.vis.rendering_color = (1.0, 0.0, 0.0)
#================= End snippet ==========================
