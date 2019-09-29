.. _data_model_intro:

===================================
Data model
===================================

OVITO organizes the data it processes into *data objects*, each representing a specific fragment of a dataset.
For example, a dataset may be composed of a :py:class:`~ovito.data.SimulationCell` object holding the cell dimensions,
a :py:class:`~ovito.data.Particles` object storing the particle information, and a :py:class:`~ovito.data.Bonds` sub-object storing the
bonds between particles. For each type of data object you will find a corresponding Python class in the :py:mod:`ovito.data` module.
All of them derive from the :py:class:`~ovito.data.DataObject` base class.

Data objects can contain other data objects. For example, the :py:class:`~ovito.data.Particles` object holds
a number of :py:class:`~ovito.data.Property` objects, one for each particle property that has been defined.
The :py:class:`~ovito.data.Particles` object can also contain a :py:class:`~ovito.data.Bonds` object, which in turn is a container for
a set of :py:class:`~ovito.data.Property` objects storing the per-bond properties:

.. image:: graphics/data_objects.*
   :width: 80 %
   :align: center

The top-level container for all other data objects is the :py:class:`~ovito.data.DataCollection` class.
It is the basic unit that gets processed by a data pipeline, i.e., it is loaded from an input file, flows down the data pipeline
and is processed by modifiers. Modifiers may alter individual data objects within a :py:class:`~ovito.data.DataCollection`,
add new data objects to the collection, or insert additional sub-objects into nested containers.

When you call the :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` method, you receive back a :py:class:`~ovito.data.DataCollection`
holding the computation results of the pipeline. The :py:class:`~ovito.data.DataCollection` class provides various property fields for accessing the different kinds
of sub-objects.

It is important to note that a :py:class:`~ovito.data.DataCollection` object represents just a single animation frame
and not an entire animation sequence. Thus, in OVITO's data model, a simulation trajectory is rather represented as a series of
:py:class:`~ovito.data.DataCollection` instances. A data pipeline operates on and produces only a single :py:class:`~ovito.data.DataCollection`
at a time, i.e., it works on a frame by frame basis.

.. _particle_properties_intro:

-----------------------------------
Particle systems
-----------------------------------

The :py:class:`~ovito.data.Particles` data object, which is accessible through the :py:attr:`DataCollection.particles <ovito.data.DataCollection.particles>`
field, holds all particle and molecule-related data. OVITO uses a property-centered representation of particles, where information is stored as a set of uniform memory arrays, all being of the same length.
Each array represents one particle property such as position, type, mass, color, etc., and holds the values for all *N* particles
in the system. A property data array is an instance of the :py:class:`~ovito.data.Property` data object class, which is not only used by OVITO for storing
particle properties but also bond properties, voxel grid properties, and more.

Thus, a system of particles is nothing else than a loose collection of :py:class:`~ovito.data.Property` objects, which are held
together by a container, the :py:class:`~ovito.data.Particles` object, which is a specialization of the generic :py:class:`~ovito.data.PropertyContainer`
base class. Each particle property has a unique name that identifies the meaning
of the property. OVITO defines a set of :ref:`standard property names <particle-types-list>`, which have a specific meaning to the program and a prescribed data format.
The ``Position`` standard property, for example, holds the XYZ coordinates of all particles and is mandatory. Other standard
properties, such as ``Color`` or ``Mass``, are optional and may or may not be present in a :py:class:`~ovito.data.Particles` container.
Furthermore, :py:class:`~ovito.data.Property` objects with non-standard names are supported, representing user-defined particle properties.

.. image:: graphics/particles_object.*
   :width: 35 %
   :align: center

The :py:class:`~ovito.data.Particles` container object mimics the interface of a Python dictionary, which lets you look up properties by name.
To find out which properties are present, you can query the dictionary for its keys::

    >>> data = pipeline.compute()
    >>> list(data.particles.keys())
    ['Particle Identifier', 'Particle Type', 'Position', 'Color']

Individual particle properties can be looked up by their name::

    >>> color_property = data.particles['Color']

Some standard properties can also be accessed through convenient getter attributes defined in the :py:class:`~ovito.data.Particles` class::

    >>> color_property = data.particles.colors

The :py:class:`~ovito.data.Particles` class is a sub-class of the more general
:py:class:`~ovito.data.PropertyContainer` base class. OVITO defines more property container types, such as the :py:class:`~ovito.data.Bonds`,
:py:class:`~ovito.data.DataSeries` and :py:class:`~ovito.data.VoxelGrid` types, which all work similar to the :py:class:`~ovito.data.Particles` type.
They all have in common that they represent an array of uniform data elements, which may be associated with a variable sets of properties.

-------------------------------------------------
Property objects
-------------------------------------------------

Each :py:class:`~ovito.data.Property` in a :py:class:`~ovito.data.PropertyContainer` stores the values of one particular property
for all data elements, for example all particles. A :py:class:`~ovito.data.Property` behaves pretty much like a standard NumPy array::

    >>> coordinates = data.particles.positions

    >>> print(coordinates[...])
    [[ 73.24230194  -5.77583981  -0.87618297]
     [-49.00170135 -35.47610092 -27.92519951]
     [-50.36349869 -39.02569962 -25.61310005]
     ...,
     [ 42.71210098  59.44919968  38.6432991 ]
     [ 42.9917984   63.53770065  36.33330154]
     [ 44.17670059  61.49860001  37.5401001 ]]

Property arrays can be one-dimensional (in case of scalar properties) or two-dimensional (in case of vector properties).
The size of the first array dimension is always equal to the number of data elements (e.g. particles) stored in the parent :py:class:`~ovito.data.PropertyContainer`.
The container reports the current number of elements via its :py:attr:`~ovito.data.PropertyContainer.count` attribute::

    >>> data.particles.count  # This returns the number of particles
    28655
    >>> data.particles['Mass'].shape   # 1-dim. array
    (28655,)
    >>> data.particles['Color'].shape  # 2-dim. array
    (28655, 3)
    >>> data.particles['Color'].dtype  # Property data type
    float64

OVITO currently supports three different numeric data types for property arrays: ``float64``, ``int32`` and ``int64``. For built-in standard properties
the data type and the dimensionality are prescribed by OVITO. For user-defined properties they can be chosen by the user when
:py:ref:`creating a new property <creating_new_properties>`.

-------------------------------------------------
Global attributes
-------------------------------------------------

...

-------------------------------------------------
Next topic
-------------------------------------------------

  * :ref:`data_manipulation_intro`
