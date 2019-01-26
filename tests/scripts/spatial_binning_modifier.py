from ovito.io import import_file
from ovito.modifiers import SpatialBinningModifier
import numpy as np

pipeline = import_file("../files/NetCDF/sheared_aSi.nc")
modifier = SpatialBinningModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  bin_count_x: {}".format(modifier.bin_count_x))
modifier.bin_count_x = 50

print("  bin_count_y: {}".format(modifier.bin_count_y))
modifier.bin_count_y = 80

print("  bin_count_z: {}".format(modifier.bin_count_z))
modifier.bin_count_z = 80

print("  reduction_operation: {}".format(modifier.reduction_operation))
modifier.reduction_operation = SpatialBinningModifier.Operation.Mean

print("  first_derivative: {}".format(modifier.first_derivative))
modifier.first_derivative = False

print("  reduction_operation: {}".format(modifier.direction))
modifier.direction = SpatialBinningModifier.Direction.XZ

print("  property: {}".format(modifier.property))
modifier.property = "Position.X"

# Check 1D binning.
modifier.direction = SpatialBinningModifier.Direction.Z
data = pipeline.compute(3)
series = data.series['binning[Position.X]']
assert(series.count == modifier.bin_count_x)
assert(series.interval_end == np.linalg.norm(data.cell[:,2]))

# Check 2D binning.
modifier.direction = SpatialBinningModifier.Direction.XZ
data = pipeline.compute()
grid = data.grids['binning[Position.X]']
assert(grid.domain.is2D)
assert(np.all(grid.domain[:,3] == data.cell[:,3]))
assert(np.all(grid.domain[:,0] == data.cell[:,0]))
assert(np.all(grid.domain[:,1] == data.cell[:,2]))
assert(grid.shape[0] == modifier.bin_count_x)
assert(grid.shape[1] == modifier.bin_count_y)
assert(grid.shape[2] == 1)

# Check 3D binning.
modifier.direction = SpatialBinningModifier.Direction.XYZ
data = pipeline.compute()
grid = data.grids['binning[Position.X]']
assert(not grid.domain.is2D)
assert(np.all(grid.domain[...] == data.cell[...]))
assert(grid.shape[0] == modifier.bin_count_x)
assert(grid.shape[1] == modifier.bin_count_y)
assert(grid.shape[2] == modifier.bin_count_z)
