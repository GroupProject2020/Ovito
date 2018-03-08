import re
try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections
import PyQt5.QtCore
import numpy

# Load dependencies
import ovito
from ovito.data import DataCollection
from ovito.plugins.PyScript import CloneHelper

# Load the native code module
from ovito.plugins.Particles import ParticleProperty

# Helper class that is part of the implementation of the DataCollection.particles attribute.
class ParticlesView(collections.Mapping):
    """
    A dictionary view of all :py:class:`ParticleProperty` objects in a :py:class:`DataCollection`.
    An instance of this class is returned by :py:attr:`DataCollection.particles`.

    It implements the ``collections.abc.Mapping`` interface. That means it can be used
    like a standard read-only Python ``dict`` object to access the particle properties by name, e.g.:

    .. literalinclude:: ../example_snippets/particles_view.py
        :lines: 7-11

    New particle properties can be added with the :py:meth:`.create_property` method.
    """

    def __init__(self, data_collection):
        self._data = data_collection

    def __len__(self):
        # Count the number of ParticleProperty objects in the collection.
        return sum(isinstance(obj, ParticleProperty) for obj in self._data.objects)

    def __getitem__(self, key):
        for obj in self._data.objects:
            if isinstance(obj, ParticleProperty): 
                if obj.name == key:
                    return obj
        raise KeyError("The DataCollection does not contain a particle property with the name '%s'." % key)
    
    def __iter__(self):
        for obj in self._data.objects:
            if isinstance(obj, ParticleProperty):
                yield obj.name
    
    # This is here for backward compatibility with OVITO 2.9.0.
    # It maps the properties to Python attributes.
    def __getattr__(self, name):
        for obj in self._data.objects:
            if isinstance(obj, ParticleProperty): 
                if obj.type != ParticleProperty.Type.User and re.sub('\W|^(?=\d)','_', obj.name).lower() == name:
                    return obj
        raise AttributeError("DataCollection does not contain the particle property '%s'." % name)
    
    def __repr__(self):
        return repr(dict(self))

    @property
    def count(self):
        """ This read-only attribute returns the number of particles in the :py:class:`DataCollection`. """
        for obj in self._data.objects:
            if isinstance(obj, ParticleProperty) and obj.type == ParticleProperty.Type.Position:
                return obj.size
        return 0

    def create_property(self, name, dtype=None, components=None, data=None):
        """
        Adds a new particle property to the data collection and optionally initializes it with 
        the per-particle data provided by the *data* parameter. The method returns the new :py:class:`ParticleProperty` 
        instance.

        The method allows to create *standard* as well as *user-defined* particle properties. 
        To create a *standard* particle property, one of the :ref:`standard property names <particle-types-list>` must be provided as *name* argument:
        
        .. literalinclude:: ../example_snippets/particles_view.py
            :lines: 16-17
        
        The length of the provided *data* array must match the number of particles, which is given by the :py:attr:`.count` attribute.
        You can also set the values of the property after its construction: 

        .. literalinclude:: ../example_snippets/particles_view.py
            :lines: 23-25

        To create a *user-defined* particle property, use a non-standard property name:
        
        .. literalinclude:: ../example_snippets/particles_view.py
            :lines: 29-30
        
        In this case the data type and the number of vector components of the new property are inferred from
        the provided *data* Numpy array. Providing a one-dimensional array creates a scalar property while
        a two-dimensional array creates a vectorial property.
        Alternatively, the *dtype* and *components* parameters can be specified explicitly
        if initialization of the property values should happen after property creation:

        .. literalinclude:: ../example_snippets/particles_view.py
            :lines: 34-36

        If the property to be created already exists in the data collection, it is replaced with a new one.
        The existing per-particle data from the old property is however retained if *data* is ``None``.

        Note: If the data collection contains no particles yet, that is, even the ``Position`` property
        is not present in the data collection yet, then the ``Position`` standard property can still be created from scratch as a first particle property by the 
        :py:meth:`!create_property` method. The *data* array has to be provided in this case to specify the number of particles
        to create:

        .. literalinclude:: ../example_snippets/particles_view.py
            :lines: 40-45
        
        After the initial ``Positions`` property has been created, the number of particles is now specified and any subsequently added properties 
        must have the exact same length.

        :param name: Either a :ref:`standard property type constant <particle-types-list>` or a name string.
        :param data: An optional data array with per-particle values for initializing the new property.
                     The size of the array must match the particle :py:attr:`.count` in the data collection
                     and the shape must be consistent with the number of components of the property.
        :param dtype: The element data type when creating a user-defined property. Must be either ``int`` or ``float``.
        :param int components: The number of vector components when creating a user-defined property.
        :returns: The newly created :py:class:`ParticleProperty`
        """

        # Input parameter validation and inferrence:
        if isinstance(name, ParticleProperty.Type):
            if name == ParticleProperty.Type.User:
                raise TypeError("Invalid standard property type.")
            property_type = name
        else:
            property_name = name
            property_type = ParticleProperty.standard_property_type_id(property_name)

        if property_type != ParticleProperty.Type.User:
            if components is not None:
                raise ValueError("Specifying vector components for a standard property is not allowed.")
            if dtype is not None:
                raise ValueError("Specifying a data type for a standard property is not allowed.")
        else:
            if components is None or dtype is None:
                if data is None: raise ValueError("Must provide a 'data' array if data type or component count are not specified.")
            if data is not None:
                data = numpy.asanyarray(data)
                if data.ndim < 1 or data.ndim > 2:
                    raise ValueError("Provided data array must be either 1 or 2-dimensional")
            if components is None:
                components = data.shape[1] if data.ndim==2 else 1
            if dtype is None:
                dtype = data.dtype
            if components < 1:
                raise ValueError("Invalid number of vector components for a user-defined property.")
            if not property_name or '.' in property_name:
                raise ValueError("Invalid property name: {}".format(property_name))

            # Translate data type from Python to Qt metatype id.
            if dtype == int or dtype == numpy.int_:
                dtype = PyQt5.QtCore.QMetaType.Int
            elif dtype == numpy.longlong or dtype == numpy.int64:
                dtype = PyQt5.QtCore.QMetaType.LongLong
            elif dtype == float:
                dtype = PyQt5.QtCore.QMetaType.type('FloatType')
            else:
                raise TypeError("Invalid property data type. Only 'int', 'int64' or 'float' are allowed.")
        
        # Check if property already exists in the data collection.
        # Also look up the 'Position' particle property to determine the number of particles.
        existing_prop = None
        position_prop = None
        for obj in self._data.objects:
            if isinstance(obj, ParticleProperty):
                if obj.type == ParticleProperty.Type.Position:
                    position_prop = obj
                if property_type != ParticleProperty.Type.User and obj.type == property_type:
                    existing_prop = obj
                elif property_type == ParticleProperty.Type.User and obj.name == property_name:
                    existing_prop = obj

        # First determine the number of particles from the 'Position' particle property.
        if position_prop is None:
            if property_type != ParticleProperty.Type.Position or data is None:
                raise RuntimeError("Cannot create particle property, because the data collection contains no particles yet.")
            else:
                num_particles = len(data)
        else:
            num_particles = position_prop.size

        # Check data array dimensions.
        if data is not None:
            if len(data) != num_particles:
                raise ValueError("Array size mismatch. Length of data array must match the number of particles in the data collection.")
                    
        if existing_prop is None:
            # If property does not exists yet, create a new ParticleProperty instance.
            if property_type != ParticleProperty.Type.User:
                prop = ParticleProperty.createStandardProperty(ovito.dataset, num_particles, property_type, data is None)
            else:
                prop = ParticleProperty.createUserProperty(ovito.dataset, num_particles, dtype, components, 0, property_name, data is None)

            # Initialize property with per-particle data if provided.
            if data is not None:
                with prop:
                    prop[...] = data
            
            # Insert new property into data collection.
            self._data.objects.append(prop)
        else:
            # Make sure the data layout of the existing property is compatible with the requested layout.
            if components is not None and existing_prop.components != components:
                raise ValueError("Existing property '{}' has {} vector component(s), but {} component(s) have been requested for the new property.".format(existing_prop.name, existing_prop.components, components))
            if dtype is not None and existing_prop.data_type != dtype:
                raise ValueError("Existing property '{}' has data type '{}', but data type '{}' has been requested for the new property.".format(
                    existing_prop.name, PyQt5.QtCore.QMetaType.typeName(existing_prop.data_type), PyQt5.QtCore.QMetaType.typeName(dtype)))
            
            # Make a copy of the existing property so that the caller can safely modify it.
            idx = self._data.objects.index(existing_prop)
            prop = CloneHelper().clone(existing_prop, False)

            # Initialize property with per-particle data if provided.
            if data is not None:
                with prop:
                    prop[...] = data
            
            # Replace old property in data collection with new clone.
            self._data.objects[idx] = prop
        
        return prop

    # Here only for backward compatibility with OVITO 2.9.0:
    def create(self, name, dtype=None, components=None, data=None):
        return self.create_property(name, dtype=dtype, components=components, data=data)
