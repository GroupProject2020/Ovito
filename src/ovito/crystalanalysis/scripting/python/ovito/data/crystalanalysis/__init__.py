# Load dependencies
import ovito.data.stdobj
import ovito.data.mesh
import ovito.data.grid
import ovito.data.stdmod
import ovito.data.particles
import numpy

# Load the native code module
from ovito.plugins.CrystalAnalysisPython import DislocationNetwork, DislocationSegment

# Load submodules.
from .data_collection import DataCollection

# Inject classes into parent module.
ovito.data.DislocationNetwork = DislocationNetwork
ovito.data.DislocationSegment = DislocationSegment
ovito.data.__all__ += ['DislocationNetwork', 'DislocationSegment']
