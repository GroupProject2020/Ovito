from ovito.io import import_file
from ovito.modifiers import HistogramModifier
import numpy

pipeline = import_file("../files/NetCDF/sheared_aSi.nc")

modifier = HistogramModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  bin_count: {}".format(modifier.bin_count))
modifier.bin_count = 50

print("  fix_xrange: {}".format(modifier.fix_xrange))
modifier.fix_xrange = True

print("  xrange_start: {}".format(modifier.xrange_start))
modifier.xrange_start = 0

print("  xrange_end: {}".format(modifier.xrange_end))
modifier.xrange_end = 50

print("  particle_property: {}".format(modifier.particle_property))
modifier.particle_property = "Position.X"

print("  bond_property: {}".format(modifier.bond_property))
modifier.bond_property = "Length"

print("  source_mode: {}".format(modifier.source_mode))
modifier.source_mode = HistogramModifier.SourceMode.Particles

print("  property: {}".format(modifier.property))
modifier.property = "Position.X"

data = pipeline.compute()

print("Output:")

histogram = data.series["histogram[Position.X]"]
assert(histogram.count == modifier.bin_count)
assert(histogram.interval_start == modifier.xrange_start)
assert(histogram.interval_end == modifier.xrange_end)

print("  xrange_start: {}".format(modifier.xrange_start))
print("  xrange_end: {}".format(modifier.xrange_end))

#numpy.savetxt("histogram.txt", modifier.histogram)
