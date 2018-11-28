# Load dependencies
import ovito.data
import ovito.data.stdobj
import ovito.data.stdmod
import ovito.data.mesh
import ovito.data.grid

# Load the native code module
import ovito.plugins.Particles

# Load submodules.
from .cutoff_neighbor_finder import CutoffNeighborFinder
from .nearest_neighbor_finder import NearestNeighborFinder
from .data_collection import DataCollection
from .particles_object import Particles
from .bonds_object import Bonds
from .trajectory_lines import TrajectoryLines

# Inject selected classes into parent module.
ovito.data.BondsEnumerator = ovito.plugins.Particles.BondsEnumerator
ovito.data.ParticleType = ovito.plugins.Particles.ParticleType
ovito.data.BondType = ovito.plugins.Particles.BondType
ovito.data.CutoffNeighborFinder = CutoffNeighborFinder
ovito.data.NearestNeighborFinder = NearestNeighborFinder
ovito.data.Particles = Particles
ovito.data.Bonds = Bonds
ovito.data.TrajectoryLines = TrajectoryLines
ovito.data.__all__ += ['ParticleType',
            'BondType', 'BondsEnumerator', 
            'CutoffNeighborFinder', 'NearestNeighborFinder',
            'Particles', 'Bonds', 'TrajectoryLines']
