# Load system libraries
import numpy

# Load dependencies
import ovito
import ovito.modifiers

# Load the native code module
from ovito.plugins.StdModPython import HistogramModifier

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
