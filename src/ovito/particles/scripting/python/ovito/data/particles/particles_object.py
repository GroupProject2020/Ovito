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

# Particle creation function.
def _Particles_create_particle(self, position):
    """
    Adds a new particle to the particle system.

    :param tuple position: The xyz coordinates for the new particle.
    :returns: The index of the newly created particle, i.e. :py:attr:`(Particles.count-1) <ovito.data.PropertyContainer.count>`.
    """
    assert(len(position) == 3)
    particle_index = self.count # Index of the newly created particle.

    # Extend the particles array by 1:
    self.set_element_count(particle_index + 1)
    # Store the coordinates in the 'Position' particle property:
    self.make_mutable(self.create_property("Position"))[particle_index] = position

    return particle_index
Particles.create_particle = _Particles_create_particle

# Implementation of the Particles.delta_vector() method.
def _Particles_delta_vector(self, a, b, cell, return_pbcvec=False):
    """
    Computes the vector connecting two particles *a* and *b* in a periodic simulation cell by applying the minimum image convention.

    This is a convenience wrapper for the :py:meth:`SimulationCell.delta_vector() <ovito.data.SimulationCell.delta_vector>` method,
    which computes the vector between two arbitrary spatial locations :math:`r_a` and :math:`r_b` taking into account periodic
    boundary conditions. The version of the method described here takes two particle indices *a* and *b* as input, computing the shortest vector
    :math:`{\Delta} = (r_b - r_a)` between them using the `minimum image convention <https://en.wikipedia.org/wiki/Periodic_boundary_conditions>`_.
    Please see the :py:meth:`SimulationCell.delta_vector() <ovito.data.SimulationCell.delta_vector>` method for further information.

    :param a: Zero-based index of the first input particle. This may also be an array of particle indices.
    :param b: Zero-based index of the second input particle. This may also be an array of particle indices with the same length as *a*.
    :param cell: The :py:class:`~ovito.data.SimulationCell` object that defines the periodic domain. Typically, :py:attr:`DataCollection.cell <ovito.data.DataCollection.cell>` is used here.
    :param bool return_pbcvec: If True, also returns the vector :math:`n`, which specifies how often the computed particle-to-particle vector crosses the cell's face.
    :returns: The delta vector and, optionally, the vector :math:`n`.

    """
    return cell.delta_vector(self.positions[a], self.positions[b], return_pbcvec)
Particles.delta_vector = _Particles_delta_vector