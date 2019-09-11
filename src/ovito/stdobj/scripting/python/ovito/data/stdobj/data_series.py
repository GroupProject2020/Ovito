import numpy

# Load dependencies
import ovito

# Load the native code module
from ovito.plugins.StdObjPython import DataSeries

# Implementation of the DataSeries.interval property.
def _get_DataSeries_interval(self):
    """ 
    A tuple of float values specifying the x-axis interval covered by the data series. 
    This information is only used if the data points do not have an explicit :py:attr:`.x` coordinate property.
    This is typically the case for data series that represent histograms, which have evenly-sized bins
    covering a certain interval along the x-axis. The bin size is then given by the total interval length divided by the
    number of data points (see :py:attr:`PropertyContainer.count` property).
    """
    return (self.interval_start, self.interval_end)
def _set_DataSeries_interval(self, interval):
    if len(interval) != 2: raise TypeError("Tuple or list of length 2 expected.")
    self.interval_start = range[0]
    self.interval_end = range[1]
DataSeries.interval = property(_get_DataSeries_interval, _set_DataSeries_interval)

# Implementation of the DataSeries.as_table() method.
def _DataSeries_as_table(self):
    """
    Returns a NumPy array containing both the x- and the y-coordinates of the data points of this data series.
    If the data series does not have an explicit :py:attr:`.x` coordinate property, this method will 
    compute the x-coordinates on the fly from the :py:attr:`.interval` set for this data series. 
    """
    x = self.x
    y = self.y
    if not x:
        half_step_size = 0.5 * (self.interval_end - self.interval_start) / len(y)
        x = numpy.linspace(half_step_size, self.interval_end - half_step_size, num = len(y))
    if x.ndim == 1: x = x[:,numpy.newaxis]
    if y.ndim == 1: y = y[:,numpy.newaxis]
    return numpy.hstack((x, y))
DataSeries.as_table = _DataSeries_as_table
