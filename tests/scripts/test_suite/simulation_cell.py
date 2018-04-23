from ovito.io import import_file
from ovito.data import SimulationCell
import numpy

pipeline = import_file("../../files/LAMMPS/animation.dump.gz")
data = pipeline.compute()
cell = data.expect(SimulationCell)

print("  input pbc flags: {}".format(cell.pbc))
cell.pbc = (False, True, True)

print("  input cell:\n{}".format(cell))
with cell:
    cell[...] = [[10,0,0,0],[0,2,0,0],[0,0,1,0]]

assert(cell[0,0] == 10)
assert(cell[1,1] == 2)

print("Output cell:")
print(cell)
print("pbc:", cell.pbc)
print("volume:", cell.volume)
print("volume2D:", cell.volume2D)
print("is2D:", cell.is2D)
print("asarray:")
a = numpy.asarray(cell)
print(a, type(a), a.shape, a.dtype)
