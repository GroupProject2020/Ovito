# Load dependencies
import ovito
import ovito.data.mesh
import ovito.data.grid
import ovito.data.stdobj
import ovito.data.stdmod

# Load the native code module
from ovito.plugins.ParticlesPython import Bonds

from ovito.data.stdobj import PropertyContainer

Bonds.topology  = PropertyContainer._create_property_accessor("Topology", "The :py:class:`~ovito.data.Property` data array for the ``Topology`` standard bond property; or ``None`` if that property is undefined.")
Bonds.topology_ = PropertyContainer._create_property_accessor("Topology_")

Bonds.pbc_vectors  = PropertyContainer._create_property_accessor("Periodic Image", "The :py:class:`~ovito.data.Property` data array for the ``Periodic Image`` standard bond property; or ``None`` if that property is undefined.")
Bonds.pbc_vectors_ = PropertyContainer._create_property_accessor("Periodic Image_")

Bonds.colors  = PropertyContainer._create_property_accessor("Color", "The :py:class:`~ovito.data.Property` data array for the ``Color`` standard bond property; or ``None`` if that property is undefined.")
Bonds.colors_ = PropertyContainer._create_property_accessor("Color_")

Bonds.selection  = PropertyContainer._create_property_accessor("Selection", "The :py:class:`~ovito.data.Property` data array for the ``Selection`` standard bond property; or ``None`` if that property is undefined.")
Bonds.selection_ = PropertyContainer._create_property_accessor("Selection_")

Bonds.bond_types  = PropertyContainer._create_property_accessor("Bond Type", "The :py:class:`~ovito.data.Property` data array for the ``Bond Type`` standard bond property; or ``None`` if that property is undefined.")
Bonds.bond_types_ = PropertyContainer._create_property_accessor("Bond Type_")

# Bond creation function.
def _Bonds_create_bond(self, a, b, type=None, pbcvec=None):
    """
    Creates a new bond between two particles *a* and *b*, both parameters being indices into the :py:class:`~ovito.data.Particles` list
    to which this :py:class:`!Bonds` container belongs.

    :param int a: Index of first particle connected by the new bond. Particle indices start at 0.
    :param int b: Index of second particle connected by the new bond.
    :param int type: Optional type ID to be assigned to the new bond. This value will be stored to the :py:attr:`bond_types` array.
    :param tuple pbcvec: Three integers specifying the bond's crossings of periodic cell boundaries. The information will be stored in the :py:attr:`pbc_vectors` array.
    :returns: The index of the newly created bond, i.e. :py:attr:`(Bonds.count-1) <ovito.data.PropertyContainer.count>`.

    The method does *not* check if there already is an existing bond connecting the same pair of particles.

    The method does *not* check if the particle indices *a* and *b* do exist. Thus, it is your responsibility to ensure that both indices
    are in the range 0 to :py:attr:`(Particles.count-1) <ovito.data.PropertyContainer.count>`.

    In case the :py:class:`~ovito.data.SimulationCell` has periodic boundary conditions enabled, and the two particles connected by the bond are located in different periodic images,
    make sure you provide the *pbcvec* argument. It is required so that OVITO does not draw the bond as a direct line from particle *a* to particle *b* but as a line passing through
    the periodic cell faces. You can use the :py:meth:`Particles.delta_vector() <ovito.data.Particles.delta_vector>` function to compute
    *pbcvec* or use the ``pbc_shift`` vector returned by the :py:class:`~ovito.data.CutoffNeighborFinder` utility.
    """
    bond_index = self.count # Index of the newly created bond.

    # Extend the bonds array by 1:
    self.set_element_count(bond_index + 1)
    # Store the indices (a,b) in the 'Topology' bond property:
    self.make_mutable(self.create_property("Topology"))[bond_index] = (a,b)
    # Assign other bond properties:
    if type is not None:
        self.make_mutable(self.create_property("Bond Type"))[bond_index] = type
    if pbcvec is not None:
        self.make_mutable(self.create_property("Periodic Image"))[bond_index] = pbcvec

    return bond_index
Bonds.create_bond = _Bonds_create_bond