# Load dependencies
import ovito
import ovito.data
import ovito.data.mesh
import ovito.data.stdobj

# Load the native code module
import ovito.plugins.Particles

# Load submodules.
from .cutoff_neighbor_finder import CutoffNeighborFinder
from .nearest_neighbor_finder import NearestNeighborFinder
from .data_collection import DataCollection

# Inject selected classes into parent module.
ovito.data.BondsEnumerator = ovito.plugins.Particles.BondsEnumerator
ovito.data.ParticleType = ovito.plugins.Particles.ParticleType
ovito.data.BondType = ovito.plugins.Particles.BondType
ovito.data.CutoffNeighborFinder = CutoffNeighborFinder
ovito.data.NearestNeighborFinder = NearestNeighborFinder
ovito.data.Particles = ovito.plugins.Particles.Particles
ovito.data.Bonds = ovito.plugins.Particles.Bonds
ovito.data.TrajectoryLines = ovito.plugins.Particles.TrajectoryLines
ovito.data.__all__ += ['ParticleType',
            'BondType', 'BondsEnumerator', 
            'CutoffNeighborFinder', 'NearestNeighborFinder',
            'Particles', 'Bonds', 'TrajectoryLines']
