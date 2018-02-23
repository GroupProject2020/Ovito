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
from .particle_property import ParticleProperty
from .bond_property import BondProperty
from .data_collection import DataCollection
from .particles_view import ParticlesView
from .bonds_view import BondsView

# Inject selected classes into parent module.
ovito.data.ParticleProperty = ParticleProperty
ovito.data.BondsEnumerator = ovito.plugins.Particles.BondsEnumerator
ovito.data.ParticleType = ovito.plugins.Particles.ParticleType
ovito.data.BondProperty = BondProperty
ovito.data.BondType = ovito.plugins.Particles.BondType
ovito.data.CutoffNeighborFinder = CutoffNeighborFinder
ovito.data.NearestNeighborFinder = NearestNeighborFinder
ovito.data.ParticlesView = ParticlesView
ovito.data.BondsView = BondsView
ovito.data.__all__ += ['ParticleProperty', 'ParticleType',
            'BondProperty', 'BondType', 'BondsEnumerator', 
            'CutoffNeighborFinder', 'NearestNeighborFinder',
            'ParticlesView', 'BondsView']

# For backward compatibility with OVITO 2.9.0:
ovito.data.Bonds = BondsView
