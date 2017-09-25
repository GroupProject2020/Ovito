# Load system libraries
import numpy

# Load dependencies
import ovito
import ovito.modifiers

# Load the native code module
from ..plugins.PyScript import HistogramModifier

# Implementation of the HistogramModifier.histogram attribute.
def _HistogramModifier_histogram(self):
    """
    Returns a NumPy array containing the histogram computed by the modifier.    
    The returned array is two-dimensional and consists of [*x*, *count(x)*] value pairs, where
    *x* denotes the bin center and *count(x)* the number of particles whose property value falls into the bin.
    
    Note that accessing this array is only possible after the modifier has computed its results. 
    Thus, you have to call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that the histogram was generated.
    """
    # Get bin counts
    ydata = self._histogram_data
    if len(ydata) != self.bin_count:
        raise RuntimeError("The modifier has not computed its results yet.")
    # Compute bin center positions
    binsize = (self.xrange_end - self.xrange_start) / len(ydata)
    xdata = numpy.linspace(self.xrange_start + binsize * 0.5, self.xrange_end + binsize * 0.5, len(ydata), endpoint = False)
    return numpy.transpose((xdata,ydata))
HistogramModifier.histogram = property(_HistogramModifier_histogram)


# For backward compatibility with OVITO 2.9.0:
def _HistogramModifier_set_particle_property(self, v): self.property = v
HistogramModifier.particle_property = property(lambda self: self.property, _HistogramModifier_set_particle_property)
def _HistogramModifier_set_bond_property(self, v): 
    self.operate_on = 'bonds'
    self.property = v
HistogramModifier.bond_property = property(lambda self: self.property, _HistogramModifier_set_bond_property)
class _HistogramModifier_SourceMode:
    Particles = 'particles'
    Bonds = 'bonds'
HistogramModifier.SourceMode = _HistogramModifier_SourceMode
def _HistogramModifier_set_source_mode(self, v): self.operate_on = v
HistogramModifier.source_mode = property(lambda self: self.operate_on, _HistogramModifier_set_source_mode)
