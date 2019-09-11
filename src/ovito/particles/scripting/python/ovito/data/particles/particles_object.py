# Load dependencies
import ovito
import ovito.data.mesh
import ovito.data.grid
import ovito.data.stdobj
import ovito.data.stdmod

# Load the native code module
from ovito.plugins.ParticlesPython import Particles

from ovito.data.stdobj import PropertyContainer

Particles.positions  = PropertyContainer._create_property_accessor("Position", "The :py:class:`~ovito.data.Property` data array for the ``Position`` standard particle property; or ``None`` if that property is undefined.")
Particles.positions_ = PropertyContainer._create_property_accessor("Position_")

Particles.colors  = PropertyContainer._create_property_accessor("Color", "The :py:class:`~ovito.data.Property` data array for the ``Color`` standard particle property; or ``None`` if that property is undefined.")
Particles.colors_ = PropertyContainer._create_property_accessor("Color_")

Particles.identifiers  = PropertyContainer._create_property_accessor("Particle Identifier", "The :py:class:`~ovito.data.Property` data array for the ``Particle Identifier`` standard particle property; or ``None`` if that property is undefined.")
Particles.identifiers_ = PropertyContainer._create_property_accessor("Particle Identifier_")

Particles.particle_types  = PropertyContainer._create_property_accessor("Particle Type", "The :py:class:`~ovito.data.Property` data array for the ``Particle Type`` standard particle property; or ``None`` if that property is undefined.")
Particles.particle_types_ = PropertyContainer._create_property_accessor("Particle Type_")

Particles.structure_types  = PropertyContainer._create_property_accessor("Structure Type", "The :py:class:`~ovito.data.Property` data array for the ``Structure Type`` standard particle property; or ``None`` if that property is undefined.")
Particles.structure_types_ = PropertyContainer._create_property_accessor("Structure Type_")

Particles.forces  = PropertyContainer._create_property_accessor("Force", "The :py:class:`~ovito.data.Property` data array for the ``Force`` standard particle property; or ``None`` if that property is undefined.")
Particles.forces_ = PropertyContainer._create_property_accessor("Force_")

Particles.selection  = PropertyContainer._create_property_accessor("Selection", "The :py:class:`~ovito.data.Property` data array for the ``Selection`` standard particle property; or ``None`` if that property is undefined.")
Particles.selection_ = PropertyContainer._create_property_accessor("Selection_")

Particles.masses  = PropertyContainer._create_property_accessor("Mass", "The :py:class:`~ovito.data.Property` data array for the ``Mass`` standard particle property; or ``None`` if that property is undefined.")
Particles.masses_ = PropertyContainer._create_property_accessor("Mass_")
