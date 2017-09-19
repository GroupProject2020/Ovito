# Load dependencies
import ovito
import ovito.data
import ovito.data.mesh

# Load the native code module
import ovito.plugins.Particles

# Load submodules.
from .cutoff_neighbor_finder import CutoffNeighborFinder
from .nearest_neighbor_finder import NearestNeighborFinder
from .bonds import Bonds
from .particle_property import ParticleProperty
from .bond_property import BondProperty
from .data_collection import DataCollection
from .particle_properties_view import ParticlePropertiesView
from .bond_properties_view import BondPropertiesView

# Inject selected classes into parent module.
ovito.data.ParticleProperty = ParticleProperty
ovito.data.Bonds = Bonds
ovito.data.BondsEnumerator = ovito.plugins.Particles.BondsEnumerator
ovito.data.ParticleType = ovito.plugins.Particles.ParticleType
ovito.data.BondProperty = BondProperty
ovito.data.BondType = ovito.plugins.Particles.BondType
ovito.data.CutoffNeighborFinder = CutoffNeighborFinder
ovito.data.NearestNeighborFinder = NearestNeighborFinder
ovito.data.ParticlePropertiesView = ParticlePropertiesView
ovito.data.BondPropertiesView = BondPropertiesView
ovito.data.__all__ += ['ParticleProperty', 'Bonds', 'ParticleType',
            'BondProperty', 'BondType', 'BondsEnumerator', 
            'CutoffNeighborFinder', 'NearestNeighborFinder',
            'ParticlePropertiesView', 'BondPropertiesView']
