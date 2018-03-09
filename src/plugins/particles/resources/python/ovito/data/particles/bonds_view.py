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
from ovito.plugins.Particles import BondProperty

# Helper class that is part of the implementation of the DataCollection.bonds attribute.
class BondsView(collections.Mapping):
    """
    This class provides a dictionary view of all :py:class:`BondProperty` objects in a :py:class:`DataCollection`.
    An instance is returned by the :py:attr:`~DataCollection.bonds` attribute of the data collection:

    .. literalinclude:: ../example_snippets/bonds_view.py
        :lines: 9-10

    The :py:attr:`.count` attribute of the :py:class:`!BondsView` class reports the number of bonds.

    **Bond properties**
    
    Bonds can possess an arbitrary set of *bond properties*, just like particles possess `particle properties <../../usage.particle_properties.html>`__. 
    The values of each bond property are stored in a separate :py:class:`BondProperty` data object. The :py:class:`!BondsView` 
    class operates like a Python dictionary and provides access to all :py:class:`BondProperty` objects based on their unique name:

    .. literalinclude:: ../example_snippets/bonds_view.py
        :lines: 15-19

    New bond properties can be added using the :py:meth:`.create_property` method. Removal of a property is possible
    by deleting it from the :py:attr:`DataCollection.objects` list.

    **Bond topology**
    
    If bonds exist in a data collection, then the ``Topology`` bond property is always present. It has the special role
    of defining the connectivity between particles in the form of a *N* x 2 array of indices into the particles list. 
    In other words, each bond is defined by a pair of particle indices. 
    
    .. literalinclude:: ../example_snippets/bonds_view.py
        :lines: 23-25
    
    Bonds are stored in no particular order. If you need to enumerate all bonds connected to a certain particle, you can use the 
    :py:class:`BondsEnumerator` utility class for that.
    
    **Bond display settings**

    The ``Topology`` bond property has a :py:class:`~ovito.vis.BondsVis` element attached to it,
    which controls the visual appearance of the bonds in rendered images. It can be accessed through the :py:attr:`~DataObject.vis` 
    attribute:
    
    .. literalinclude:: ../example_snippets/bonds_view.py
        :lines: 30-32
    
    **Computing bond vectors**
    
    Since each bond is defined by two indices into the particles array, we can use this to determine the corresponding spatial 
    bond *vectors*. They can be computed from the positions of the particles:
    
    .. literalinclude:: ../example_snippets/bonds_view.py
        :lines: 37-39
    
    Here, the first and the second column of the bonds topology array are used to index into the particle positions array.
    The subtraction of the two indexed arrays yields the list of bond vectors. Each vector in this list points
    from the first particle to the second particle of the corresponding bond.
    
    Finally, we might have to correct for the effect of periodic boundary conditions when a bond
    connects two particles on opposite sides of the box. OVITO keeps track of such cases by means of the
    the special ``Periodic Image`` bond property. It stores a shift vector for each bond, specifying the directions in which the bond
    crosses periodic boundaries. We can use this information to correct the bond vectors computed above.
    This is done by adding the product of the cell matrix and the shift vectors from the ``Periodic Image`` bond property:
    
    .. literalinclude:: ../example_snippets/bonds_view.py
        :lines: 43-44
    
    The shift vectors array is transposed here to facilitate the transformation
    of the entire array of vectors with a single 3x3 cell matrix. 
    To summarize: In the two code snippets above we have performed
    the following calculation for every bond (*a*, *b*) in parallel:
    
       v = x(b) - x(a) + dot(H, pbc)
    
    where *H* is the cell matrix and *pbc* is the bond's PBC shift vector of the form (n\ :sub:`x`, n\ :sub:`y`, n\ :sub:`z`).
    """
    
    def __init__(self, data_collection):
        self._data = data_collection
        
    def __len__(self):
        # Count the number of BondProperty objects in the collection.
        return sum(isinstance(obj, BondProperty) for obj in self._data.objects)
    
    def __getitem__(self, key):
        for obj in self._data.objects:
            if isinstance(obj, BondProperty): 
                if obj.name == key:
                    return obj
        raise KeyError("The DataCollection contains no bond property with the name '%s'." % key)
    
    def __iter__(self):
        for obj in self._data.objects:
            if isinstance(obj, BondProperty):
                yield obj.name
            
    # This is here for backward compatibility with OVITO 2.9.0.
    # It maps the properties to Python attributes.
    def __getattr__(self, name):
        for obj in self._data.objects:
            if isinstance(obj, BondProperty): 
                if obj.type != BondProperty.Type.User and re.sub('\W|^(?=\d)','_', obj.name).lower() == name:
                    return obj
        raise AttributeError("DataCollection does not contain the bond property '%s'." % name)

    def __repr__(self):
        return repr(dict(self))

    @property
    def count(self):
        """ This read-only attribute returns the number of bonds in the :py:class:`DataCollection`. 
            It always matches the lengths of all :py:class:`BondProperty` arrays in the data collection. 
        """
        for obj in self._data.objects:
            if isinstance(obj, BondProperty) and obj.type == BondProperty.Type.Topology:
                return obj.size
        return 0

    def create_property(self, name, dtype=None, components=None, data=None):
        """ 
        Adds a new bond property to the data collection and optionally initializes it with 
        the per-bond data provided by the *data* parameter. The method returns the new :py:class:`BondProperty` 
        instance.

        The method can create *standard* and *user-defined* bond properties. To create a *standard* bond property,
        one of the :ref:`standard property names <bond-types-list>` must be provided as *name* argument:
        
        .. literalinclude:: ../example_snippets/bonds_view_create_property.py
            :lines: 16-17
        
        The size of the provided *data* array must match the number of bonds in the data collection
        given by the :py:attr:`.count` attribute. You can also set the property values after construction: 

        .. literalinclude:: ../example_snippets/bonds_view_create_property.py
            :lines: 23-25

        To create a *user-defined* bond property, use a non-standard property name:
        
        .. literalinclude:: ../example_snippets/bonds_view_create_property.py
            :lines: 29-30
        
        In this case the data type and the number of vector components of the new property are inferred from
        the provided *data* Numpy array. Providing a one-dimensional array creates a scalar property while
        a two-dimensional array creates a vectorial property. 
        Alternatively, the *dtype* and *components* parameters can be specified explicitly
        if initialization of the property values should happen after property creation:

        .. literalinclude:: ../example_snippets/bonds_view_create_property.py
            :lines: 34-36

        If the property to be created already exists in the data collection, it is replaced with a new one.
        The existing per-bond data from the old property is however retained if *data* is ``None``.

        Note: If the data collection contains no bonds yet, that is, even the ``Topology`` property
        is not present in the data collection yet, then the ``Topology`` property can still be created from scratch as a first bond property by the 
        :py:meth:`!create_property` method. The *data* array has to be provided in this case to specify the number of bonds
        to create. After the initial ``Topology`` property has been created, the number of bonds is now specified and any subsequently added properties 
        must have the exact same length.

        :param name: Either a :ref:`standard property type constant <bond-types-list>` or a name string.
        :param data: An optional data array with per-bond values for initializing the new property.
                     The size of the array must match the bond :py:attr:`.count`
                     and the shape must be consistent with the number of components of the property.
        :param dtype: The element data type when creating a user-defined property. Must be either ``int`` or ``float``.
        :param int components: The number of vector components when creating a user-defined property.
        :returns: The newly created :py:class:`BondProperty`
        """

        # Input parameter validation and inferrence:
        if isinstance(name, BondProperty.Type):
            if name == BondProperty.Type.User:
                raise TypeError("Invalid standard property type.")
            property_type = name
        else:
            property_name = name
            property_type = BondProperty.standard_property_type_id(property_name)

        if property_type != BondProperty.Type.User:
            if components is not None:
                raise ValueError("Specifying vector components for a standard property not allowed.")
            if dtype is not None:
                raise ValueError("Specifying a data type for a standard property not allowed.")
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
        # Also look up the 'Topology' bond property to determine the number of bonds.
        existing_prop = None
        topology_prop = None
        for obj in self._data.objects:
            if isinstance(obj, BondProperty):
                if obj.type == BondProperty.Type.Topology:
                    topology_prop = obj
                if property_type != BondProperty.Type.User and obj.type == property_type:
                    existing_prop = obj
                elif property_type == BondProperty.Type.User and obj.name == property_name:
                    existing_prop = obj

        # First determine the number of bonds from the 'Topology' bond property.
        if topology_prop is None:
            if property_type != BondProperty.Type.Topology or data is None:
                raise RuntimeError("Cannot create bond property, because the data collection contains no bonds yet.")
            else:
                num_bonds = len(data)
        else:
            num_bonds = topology_prop.size

        # Check data array dimensions.
        if data is not None:
            if len(data) != num_bonds:
                raise ValueError("Array size mismatch. Length of data array must match the number of bonds in the data collection.")
                    
        if existing_prop is None:
            # If property does not exists yet, create a new BondProperty instance.
            if property_type != BondProperty.Type.User:
                prop = BondProperty.createStandardProperty(ovito.dataset, num_bonds, property_type, data is None)
            else:
                prop = BondProperty.createUserProperty(ovito.dataset, num_bonds, dtype, components, 0, property_name, data is None)

            # Initialize property with per-bond data if provided.
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

            # Initialize property with per-bond data if provided.
            if data is not None:
                with prop:
                    prop[...] = data
            
            # Replace old property in data collection with new clone.
            self._data.objects[idx] = prop
        
        return prop

    # This is here for backward compatibility with OVITO 2.9.0.
    @property
    def pbc_vectors(self):
        # A NumPy array providing read access to the PBC shift vectors of bonds.
            
        # The returned array's shape is *N x 3*, where *N* is the number of bonds. It contains the
        # periodic shift vector for each bond.
            
        # A PBC shift vector consists of three integers, which specify how many times (and in which direction)
        # the corresonding bond crosses the periodic boundaries of the simulation cell when going from the first particle 
        # to the second. For example, a shift vector (0,-1,0)
        # indicates that the bond crosses the periodic boundary in the negative Y direction 
        # once. In other words, the particle where the bond originates from is located
        # close to the lower edge of the simulation cell in the Y direction while the second particle is located 
        # close to the opposite side of the box.
            
        # The PBC shift vectors are important for visualizing the bonds between particles with wrapped coordinates, 
        # which are located on opposite sides of a periodic cell. When the PBC shift vector of a bond is (0,0,0), OVITO assumes that 
        # both particles connected by the bond are part of the same periodic image and the bond is rendered such that
        # it directly connects the two particles without going through a cell boundary.        
        return numpy.asarray(self['Periodic Image'])

    # This is here for backward compatibility with OVITO 2.9.0.
    @property
    def array(self):
        return numpy.asarray(self['Topology'])

    # Here only for backward compatibility with OVITO 2.9.0:
    def create(self, name, dtype=None, components=None, data=None):
        return self.create_property(name, dtype=dtype, components=components, data=data)
