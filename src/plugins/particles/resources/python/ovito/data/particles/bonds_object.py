# Load dependencies
import ovito
import ovito.data.mesh
import ovito.data.grid
import ovito.data.stdobj
import ovito.data.stdmod

# Load the native code module
from ovito.plugins.Particles import Bonds

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
