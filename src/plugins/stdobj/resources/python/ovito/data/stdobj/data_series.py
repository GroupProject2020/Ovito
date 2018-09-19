import numpy

# Load dependencies
import ovito

# Load the native code module
from ovito.plugins.StdObj import DataSeries

# Implementation of the DataSeries.as_table() method.
def _DataSeries_as_table(self):
    x = self.x
    y = self.y
    if not x:
        half_step_size = 0.5 * (self.interval_end - self.interval_start) / len(y)
        x = numpy.linspace(half_step_size, self.interval_end - half_step_size, num = len(y))
    if x.ndim == 1: x = x[:,numpy.newaxis]
    if y.ndim == 1: y = y[:,numpy.newaxis]
    return numpy.hstack((x, y))
DataSeries.as_table = _DataSeries_as_table
