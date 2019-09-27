.. _data_manipulation_intro:

===================================
Manipulating data
===================================

This section discusses how the data in a :py:class:`~ovito.data.DataCollection` can be modified or amended, for example how
you can manipulate the property values of particles or add new properties. See the :ref:`previous section <data_model_intro>` for a description of how
the data is organized in a :py:class:`~ovito.data.DataCollection` container and how it can be accessed from Python code.

.. _data_ownership:

Data ownership
-----------------------------------

OVITO's data pipeline system tries to avoid unnecessary and expensive data copies as far as possible. In order to achieve this goal, data objects
follow a shared ownership model. That means they may be part of more than one :py:class:`~ovito.data.DataCollection` at the same time.
For instance, when you duplicate an existing :py:class:`~ovito.data.DataCollection` using its :py:meth:`~ovito.data.DataCollection.clone` method,
you will obtain a second data collection containing the exact same data objects as the original collection::

    data2 = data.clone()
    assert(data2.particles is data.particles)

The assertion statement above shows that the :py:class:`~ovito.data.Particles` object in the original data collection and in the cloned
collection are physically one and the same Python object. In other words, the :py:class:`~ovito.data.Particles` data object
has not been copied but rather just been inserted into the second data collection too. It is now owned simultaneously by both data collections.

This type of "shallow" copying is used by OVITO's data pipeline system in many situations. For instance, the :py:class:`~ovito.pipeline.FileSource`
of a pipeline always keeps the original :py:class:`~ovito.data.DataCollection` that was loaded from the input file in memory.
(You can access it through the :py:attr:`FileSource.data <ovito.pipeline.FileSource.data>` field.) This unmodified master copy of the
input data enables quick re-evaluations of the data pipeline in case a modifier is being changed, without the need to reload the input data
again from disk. Modifiers in the pipeline always operate on a copy of the original :py:class:`~ovito.data.DataCollection`, and it is
the final copy that the :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` method returns to the caller. However,
it would be inefficient for the system to fully copy the master :py:class:`~ovito.data.DataCollection` including all its contents,
because modifiers typically change just a small portion of a dataset at a time, or they just amend the dataset with some newly computed information
without altering any of the existing data objects at all.

.. image:: graphics/shallow_copy_pipeline.*
   :width: 90 %
   :align: center

This is where so-called shallow object copies come into play. It is okay to share data objects between more than one data collection
as long as these data objects are not being modified. In case you intend to modify a particular data object, for example when implementing
a :ref:`user-defined modifier function <writing_custom_modifiers>`, you first have to make sure that the data object is exclusively owned
by just one :py:class:`~ovito.data.DataCollection`. Otherwise unwanted side effects would occur, because changing the data object in your
data collection would also affect the contents of another data collection owned by someone else.

.. note::

   Data objects can be part of more than one data collection (or other type of container) at a time.
   Then only read access to a shared data object is allowed, because object modifications would
   result in unexpected side effects.

.. _underscore_notation:

Announcing object modification
------------------------------------

OVITO's Python programming interface has safeguards in place that prevent you from accidentally modifying data objects
that are shared with some other part of the program. Consider the following attempt to change
the boundary condition flags of a :py:class:`~ovito.data.SimulationCell` object in a data collection returned by the pipeline system::

    >>> data = pipeline.compute()
    >>> data.cell.pbc = (True, True, False)
    RuntimeError: You tried to modify a SimulationCell object that is currently shared by
    multiple owners. Please explicitly request a mutable version of the data object by
    using the '_' notation.

The attempt to assign a new value to the cell's ``pbc`` field raised an error, because OVITO detects that the :py:class:`~ovito.data.SimulationCell`
object is not only part of the data collection returned by :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` but
also of another internal data collection owned by the pipeline. Thus, modifying the simulation cell object is not valid, because
it would cause side effects on the internal state of OVITO.

The solution is to first make a unique copy of the :py:class:`~ovito.data.SimulationCell` object before modifying it. The programing
interface provides a handy shortcut notation for this::

    >>> data.cell_.pbc = (True, True, False)

The underscore suffix appended to the :py:attr:`~ovito.data.DataCollection.cell` field signals the :py:class:`~ovito.data.DataCollection`
that your intention is to modify the simulation cell object. Behind the scenes, the data collection will check whether the cell object
is shared with multiple owners. If so, it will create an actual copy of the data object and replace the original reference in this data collection.
It thus makes sure that the returned :py:class:`~ovito.data.SimulationCell` object is exclusively owned by your data collection only, making it safe for you to modify it.

.. note::

   Before modifying a data object that is referenced by multiple owners (i.e. that is part of more than one parent container),
   the child object must be replaced with an exclusively owned copy of the original, which is safe to modify.
   OVITO's Python interface provides the underscore notation to perform this copy-and-replacement step if needed.
   You can consider the underscore suffix as a means to signal your intention to modify the object.

.. _creating_new_properties:

Assigning new particle properties
-----------------------------------------------------

New per-particle properties are assigned to a :ref:`particle system <particle_properties_intro>` using the :py:meth:`PropertyContainer.create_property() <ovito.data.PropertyContainer.create_property>`
method. In addition to the name of the property, this method accepts an optional NumPy array as argument
for initializing the per-particle values of the new property::

    color_values = numpy.random.random_sample(size=(data.particles.count, 3))
    data.particles_.create_property('Color', data=color_values)

Note that we used the underscore version of the :py:attr:`DataCollection.particles_ <ovito.data.DataCollection.particles>`
field here in order to request a modifiable version of the :py:class:`~ovito.data.Particles` container object.
This is necessary, because :py:meth:`~ovito.data.PropertyContainer.create_property` adds a new :py:class:`~ovito.data.Property`
object to the particles container object, which may be implicitly shared by multiple data collections (see previous section).

If the particle property with the given name already exists in the :py:class:`~ovito.data.Particles` container, then its contents will be overwritten
with the per-particle array provided in the NumPy array.

.. _creating_new_bond_properties:

Assigning new bond properties
-----------------------------------------------------

Like in the case of particles, new properties can also be assigned to bonds using the :py:meth:`~ovito.data.PropertyContainer.create_property`
method. Bond properties are stored in the :py:class:`~ovito.data.Bonds` container object, which is owned by the parent
:py:class:`~ovito.data.Particles` object. Thus, adding a new bond property represents a change to the nested :py:class:`~ovito.data.Bonds`
object as well as to the parent :py:class:`~ovito.data.Particles` object. Therefore we need to request modifiable versions of
both objects in this case (``particles_.bonds_.``)::

    color_values = numpy.random.random_sample(size = (data.particles.bonds.count, 3))
    data.particles_.bonds_.create_property('Color', data=color_values)

.. _modifying_properties:

Modifying property values
-----------------------------------------------------

Manipulating the values of existing particle or bond properties is as easy as changing values in a NumPy array;
just make sure you use the :py:ref:`underscore notation <underscore_notation>` introduced above to make the
property array modifiable::

    # Move the first particle to new XYZ coordinates:
    data.particles_.positions_[0] = (10.0, 8.0, 4.5)

    # Displace the first particle by 2 length units along the x-axis:
    data.particles_.positions_[0] += (2.0, 0, 0)

    # Displace all particles by 1 length unit along the z-axis:
    data.particles_.positions_[...] += (0, 0, 1.0)

You can also make use of NumPy's advanced indexing capabilities to modify just a subset of the elements::

    # Displace only selected particles, i.e. those whose 'Selection' property is non-zero:
    data.particles_.positions_[data.particles.selection != 0] += (0, 0, 1)

    # Alternatively, modify just the z-component of each position vector:
    data.particles_.positions_[data.particles.selection != 0, 2] += 1

.. _adding_global_attributes:

Adding global attributes
-----------------------------------

Global attributes are primitive values (numeric values/text strings) associated with a :py:class:`~ovito.data.DataCollection` as a whole,
for instance the simulation time of the current snapshot or the name of the input file from which the dataset was originally loaded.
Furthermore, some analysis modifiers output their computation results
as new global attributes and add them to the :py:class:`~ovito.data.DataCollection`.
Global attributes are stored within the :py:attr:`~ovito.data.DataCollection.attributes` Python dictionary
of the :py:class:`~ovito.data.DataCollection`. Thus, adding, removing, or changing attributes
is as simple as modifying a Python dictionary, e.g.::

    data.attributes['dislocation_density'] =
        data.attributes['DislocationAnalysis.total_line_length'] / data.cell.volume

This code adds the new attribute ``dislocation_density`` to the :py:class:`~ovito.data.DataCollection`, whose value is calculated from the ratio of the total dislocation
line length in a crystal (which has previously been computed by a :py:class:`~ovito.modifiers.DislocationAnalysisModifier` in this example)
and the simulation cell's :py:attr:`~ovito.data.SimulationCell.volume`.

.. note::

   When modifying the :py:attr:`~ovito.data.DataCollection.attributes` dictionary, the underscore notation is not needed,
   because the :py:attr:`~ovito.data.DataCollection.attributes` dictionary is not an object that is ever shared between more
   than one data collection.

-------------------------------------------------
Next topic
-------------------------------------------------

  * :ref:`writing_custom_modifiers`
