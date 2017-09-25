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
from .bonds import Bonds

# Helper class used to implement the DataCollection.bond_properties attribute.
class BondPropertiesView(collections.Mapping):
    """
    A dictionary view of all :py:class:`BondProperty` objects in a :py:class:`DataCollection`.
    An instance of this class is returned by :py:attr:`DataCollection.bond_properties`.

    It implements the ``collections.abc.Mapping`` interface. That means it can be used
    like a standard read-only Python ``dict`` object to access the bond properties by name, e.g.:

    .. literalinclude:: ../example_snippets/bond_properties_view.py
        :lines: 8-12

    New bond properties can be added with the :py:meth:`create` method.
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

    def create(self, name, dtype=None, components=None, data=None):
        """ 
        Adds a new bond property to the data collection and optionally initializes it with 
        the per-bond data provided by the *data* parameter. The method returns the new :py:class:`BondProperty` 
        instance.

        The method can create *standard* and *user-defined* bond properties. To create a *standard* bond property,
        one of the :ref:`standard property names <bond-types-list>` must be provided as *name* argument:
        
        .. literalinclude:: ../example_snippets/bond_properties_view.py
            :lines: 15-17
        
        The size of the provided *data* array must match the number of bond in the data collection, i.e., 
        it must equal the length of the :py:class:`Bonds` objects as well as all other bond properties that already exist in the same data collection.
        Alternatively, you can set the property values after construction: 

        .. literalinclude:: ../example_snippets/bond_properties_view.py
            :lines: 23-25

        To create a *user-defined* bond property, use a non-standard property name:
        
        .. literalinclude:: ../example_snippets/bond_properties_view.py
            :lines: 29-30
        
        In this case the data type and the number of vector components of the new property are inferred from
        the provided *data* Numpy array. Providing a one-dimensional array creates a scalar property while
        a two-dimensional array creates a vectorial property. 
        Alternatively, the *dtype* and *components* parameters can be specified explicitly
        if initialization of the property values should happen after property creation:

        .. literalinclude:: ../example_snippets/bond_properties_view.py
            :lines: 34-36

        If the property to be created already exists in the data collection, it is replaced with a new one.
        The existing per-bond data from the old property is however retained if *data* is ``None``.

        :param name: Either a :ref:`standard property type constant <bond-types-list>` or a name string.
        :param data: An optional data array with per-bond values for initializing the new property.
                     The size of the array must match the number of bonds in the data collection
                     and the shape must be consistent with the number of components of the property.
        :param dtype: The element data type when creating a user-defined property. Must be either ``int`` or ``float``.
        :param int components: The number of vector components when creating a user-defined property.
        :returns: The newly created :py:class:`~ovito.data.BondProperty`
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
            if dtype == int:
                dtype = PyQt5.QtCore.QMetaType.type('int')
            elif dtype == float:
                dtype = PyQt5.QtCore.QMetaType.type('FloatType')
            else:
                raise TypeError("Invalid property data type. Only 'int' or 'float' are allowed.")                
        
        # Check if property already exists in the data collection.
        # Also look up the Bonds data object to determine the number of bonds.
        existing_prop = None
        bonds_obj = None
        for obj in self._data.objects:
            if isinstance(obj, BondProperty):
                if property_type != BondProperty.Type.User and obj.type == property_type:
                    existing_prop = obj
                elif property_type == BondProperty.Type.User and obj.name == property_name:
                    existing_prop = obj
            elif isinstance(obj, Bonds):
                bonds_obj = obj

        # First we have to determine the number of bonds.
        if bonds_obj is None:
            raise RuntimeError("Cannot create bond property, because the data collection contains no bonds yet.")
        num_bonds = bonds_obj.size

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
                with prop.modify() as marray:
                    marray[...] = data
            
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
                with prop.modify() as marray:
                    marray[...] = data
            
            # Replace old property in data collection with new clone.
            self._data.objects[idx] = prop
        
        return prop

