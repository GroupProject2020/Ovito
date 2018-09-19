# Load dependencies
import ovito.data.mesh
import ovito.data.particles
import numpy

# Load the native code modules
import ovito.plugins.Particles
from ovito.plugins.CrystalAnalysis import DislocationNetwork, DislocationSegment, PartitionMesh

# Load submodules.
from .data_collection import DataCollection

# Inject classes into parent module.
ovito.data.DislocationNetwork = DislocationNetwork
ovito.data.DislocationSegment = DislocationSegment
ovito.data.PartitionMesh = PartitionMesh
ovito.data.__all__ += ['DislocationNetwork', 'DislocationSegment', 'PartitionMesh']
