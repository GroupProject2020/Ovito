from ovito.io import import_file
from ovito.data import SimulationCell
import numpy

pipeline = import_file("../../files/LAMMPS/animation.dump.gz")
data = pipeline.compute()
cell = data.expect(SimulationCell)

print("  input pbc flags: {}".format(cell.pbc))
mutable_cell = data.cell_
mutable_cell.pbc = (False, True, True)

print("  input cell:\n{}".format(cell))
with mutable_cell:
    mutable_cell[...] = [[10,0,0,0],[0,2,0,0],[0,0,1,0]]

assert(mutable_cell[0,0] == 10)
assert(mutable_cell[1,1] == 2)

print("Output cell:")
print(mutable_cell)
print("pbc:", mutable_cell.pbc)
print("volume:", mutable_cell.volume)
print("volume2D:", mutable_cell.volume2D)
print("is2D:", mutable_cell.is2D)
print("asarray:")
a = numpy.asarray(mutable_cell)
print(a, type(a), a.shape, a.dtype)
