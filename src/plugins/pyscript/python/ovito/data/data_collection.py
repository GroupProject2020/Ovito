import abc
try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

from ..plugins.PyScript import DataObject, SimulationCell, CloneHelper

def with_metaclass(meta, *bases):
    """
    Python 2/3 cross-compatibility function for setting a class' metaclass.
    Function from jinja2/_compat.py. License: BSD.
    """
    class metaclass(meta):
        __call__ = type.__call__
        __init__ = type.__init__
        def __new__(cls, name, this_bases, d):
            if this_bases is None:
                return type.__new__(cls, name, (), d)
            return meta(name, bases, d)
    return metaclass('temporary_class', None, {})

class DataCollection(with_metaclass(abc.ABCMeta)):
    """ 
    A :py:class:`!DataCollection` is a generic container that holds together a set of data objects, each representing different pieces of data
    such as the simulation cell, particle properties or the list of bonds. OVITO knows various types of data objects including

        * :py:class:`~ovito.data.ParticleProperty` (an array of per-particle values)
        * :py:class:`~ovito.data.SimulationCell` (stores cell vectors and boundary conditions)
        * :py:class:`~ovito.data.Bonds` (the bond connectivity)
        * :py:class:`~ovito.data.BondProperty` (an array of per-bond values)
        * :py:class:`~ovito.data.SurfaceMesh` (triangle mesh representing a two-dimensional manifold)
        * :py:class:`~ovito.data.DislocationNetwork` (discrete dislocation lines)

    All data objects of a :py:class:`!DataCollection` are stored in its :py:attr:`.objects` array. This list can hold an arbitrary number of
    data objects of arbitrary type and in arbitrary order. 
    The :py:meth:`find` and :py:meth:`find_all` methods allow to look up objects in the collection of a particular data type. For example,
    you can retrieve the :py:class:`~ovito.data.SimulationCell` instance that is typically present in a data collection using :py:meth:`.find`:

    .. literalinclude:: ../example_snippets/data_collection.py
           :lines: 7-9

    Sometimes you need to make sure that a :py:class:`~ovito.data.SimulationCell` is present in the data collection. Then you should use the :py:meth:`.expect`
    method instead of :py:meth:`.find`. It will raise an exception if there is no :py:class:`~ovito.data.SimulationCell` instance:

    .. literalinclude:: ../example_snippets/data_collection.py
           :lines: 11-12

    Additionally, the convenience accessor fields :py:attr:`.particle_properties` and :py:attr:`.bond_properties` provide filtered views of 
    just the :py:class:`~ovito.data.ParticleProperty` and :py:class:`~ovito.data.BondProperty` objects in the collection:

    .. literalinclude:: ../example_snippets/data_collection.py
           :lines: 14-15

    Note that :py:class:`!DataCollection` is an abstract base class (an *interface*). Concrete types that implement this interface are
    e.g. :py:class:`~ovito.data.PipelineFlowState`, :py:class:`~ovito.pipeline.FileSource` and :py:class:`~ovito.pipeline.StaticSource`. 
    That means all these classes are containers for data objects, but they serve different purposes within the OVITO framework. 
    The :py:class:`!DataCollection` base class provides the functionality that is common to all of them.
    """

    @staticmethod
    def registerDataCollectionType(subclass):
        # Make the class a virtual subclass of DataCollection.
        DataCollection.register(subclass)

        # Unsuccessful attribute accesses on instances of the registered class 
        # will be forwarded to the DataCollection class.
        setattr(subclass, "__getattr__", lambda self,key: getattr(DataCollection, key).__get__(self, type(self)))

    @property
    def objects(self):
        """
        The list of data objects that make up the data collection. Data objects are instances of :py:class:`DataObject`-derived
        classes, for example :py:class:`ParticleProperty`, :py:class:`Bonds` or :py:class:`SimulationCell`.

        You can add or remove objects from the :py:attr:`!objects` list to insert them or remove them from the :py:class:`!DataCollection`. 
        However, it is your responsibility to ensure that the data objects are all in a consistent state. For example,
        all :py:class:`ParticleProperty` objects in a data collection must have the same lengths at all times, because
        the length implicitly specifies the number of particles. The order in which data objects are stored in the data collection
        does not matter.

        Note that the :py:class:`!DataCollection` class also provides convenience views of the data objects contained in the :py:attr:`!objects`
        list: For example, the :py:attr:`.particle_properties` dictionary lists all :py:class:`ParticleProperty` instances in the 
        data collection by name and the :py:attr:`.bond_properties` does the same for all :py:class:`BondProperty` instances.
        Since these dictionaries are views, they always reflect the current contents of the master :py:attr:`!objects` list.
        """
        raise RuntimeError("This method should be overwritten by concrete implementations of the DataCollection interface.")

    # Implementation of the DataCollection.attributes field.
    @property
    def attributes(self):
        """
        A dictionary of key-value pairs that represent global tokens of information which are not associated with
        any specific data object in the data collection. 
        
        An *attribute* is a value of type ``int``, ``float``, or ``str`` with a unique identifier name such 
        as ``"Timestep"`` or ``"ConstructSurfaceMesh.surface_area"``. The attribute name serves as keys for the :py:attr:`!attributes` dictionary of the data collection.
        Attributes are dynamically generated by modifiers in a data pipeline or by a data source as explained in the following.
        
        **Attributes loaded from input files**

        The ``Timestep`` attribute is loaded from LAMMPS dump files and other simulation file formats
        that store the simulation timestep. Such input attributes can be retrieved from 
        the :py:attr:`!.attributes` dictionary of a pipeline's :py:attr:`~ovito.pipeline.FileSource`::

            >>> pipeline = import_file('snapshot_140000.dump')
            >>> pipeline.source.attributes['Timestep']
            140000
            
        Other attributes read from an input file are, for example, the key-value pairs found in the header line of *extended XYZ* files.
        
        **Dynamically computed attributes**
        
        Analysis modifiers like the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` or
        the :py:class:`~ovito.modifiers.ClusterAnalysisModifier` output scalar computation results
        as attributes. The reference documentation of each modifier type lists the attributes it produces.
        
        For example, the number of clusters identified by the :py:class:`~ovito.modifiers.ClusterAnalysisModifier`
        can be queried as follows::
        
            pipeline.modifiers.append(ClusterAnalysisModifier(cutoff = 3.1))
            data = pipeline.compute()
            nclusters = data.attributes["ClusterAnalysis.cluster_count"]
            
        **Exporting attributes to a text file**
        
        The :py:func:`ovito.io.export_file` function supports writing attribute values to a text
        file, possibly as functions of time::
        
            export_file(pipeline, "data.txt", "txt", 
                columns = ["Timestep", "ClusterAnalysis.cluster_count"], 
                multiple_frames = True)
                
        **User-defined attributes**
        
        The :py:class:`~ovito.modifiers.PythonScriptModifier` allows you to generate your own
        attributes that are dynamically computed (typically on the basis of some other input information):
        
        .. literalinclude:: ../example_snippets/python_modifier_generate_attribute.py
           :lines: 6-

        The :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` used in the example above generates
        the attribute ``CommonNeighborAnalysis.counts.FCC`` to report the number of atoms that 
        form an FCC lattice. To compute the fraction of FCC atoms from that, we need to divide by the total number of 
        atoms in the system. To this end, we insert a :py:class:`~ovito.modifiers.PythonScriptModifier` 
        into the pipeline behind the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier`.
        Our custom modifier function generates a new attribute named ``fcc_fraction``. Finally, 
        the value of the user-defined attribute can be queried from the pipeline or exported to 
        a text file using the :py:func:`~ovito.io.export_file` function as described above.
        """
        
        # Helper class used to implement the DataCollection.attributes field.
        class _AttributesView(collections.MutableMapping):
            
            def __init__(self, data_collection):
                """ Constructor that stores away a back-pointer to the owning DataCollection instance. """
                self._owner = data_collection
                
            def __len__(self):
                return len(self._owner.attribute_names)
            
            def __getitem__(self, key):
                v = self._owner.get_attribute(key)
                if v is not None:
                    return v
                raise KeyError("Attribute with the name '%s' not present in data collection." % key)

            def __setitem__(self, key, value):
                self._owner.set_attribute(key, value)

            def __delitem__(self, key):
                v = self._owner.get_attribute(key)
                if v is None:
                    raise KeyError("Attribute with the name '%s' not present in data collection." % key)
                self._owner.set_attribute(key, None)

            def __iter__(self):
                for aname in self._owner.attribute_names:
                    yield aname

            def __repr__(self):
                return repr(dict(self))
        
        return _AttributesView(self)

    def copy_if_needed(self, obj, deepcopy=False):
        """
        Makes a copy of a data object from this data collection if the object is not exclusively 
        owned by the data collection but shared with other collections. After the method returns,
        the data object is exclusively owned by the collection and it becomes safe to modify the object without
        causing unwanted side effects.

        Typically, this method is used in custom modifier functions (see :py:class:`~ovito.modifiers.PythonScriptModifier`) that
        participate in OVITO's data pipeline system. A modifier function receives an input collection of
        data objects from the system. However, modifying these input
        objects in place is not allowed, because they are owned by the pipeline and modifying them would 
        lead do unexpected side effects.
        This is where this method comes into play: It makes a copy of a given data object and replaces
        the original in the data collection with the copy. The caller can now safely modify this copy in place,
        because no other data collection can possibly be referring to it.

        The :py:meth:`!copy_if_needed` method first checks if *obj*, which must be a data object from this data collection, is
        shared with some other data collection. If yes, it creates an exact copy of *obj* and replaces the original
        in this data collection with the copy. Otherwise it leaves the object as is, because it is already exclusively owned
        by this data collection. 

        :param DataObject obj: The object from this data collection to be copied if needed.
        :return: An exact copy of *obj* if it was shared with some other data collection. Otherwise the original object is returned.
        """
        assert(isinstance(obj, DataObject))
        # The object to be modified must be in this data collection.
        if obj not in self.objects:
            raise ValueError("DataCollection.copy_if_needed() must be called with an object that is part of the data collection.")
        if obj.num_strong_references <= 1:
            return obj
        idx = self.objects.index(obj)
        clone = CloneHelper().clone(obj, deepcopy)
        self.objects[idx] = clone
        assert(clone.num_strong_references == 1)
        return clone

    def find(self, object_type):
        """
        Looks up the first data object from this collection of the given class type.

        :param object_type: The :py:class:`DataObject` subclass that should be looked up.
        :return: The first instance of the given class or its subclasses from the :py:attr:`.objects` list; or ``None`` if there is no instance.

        Method implementation::

            def find(self, object_type):
                for o in self.objects:
                    if isinstance(o, object_type): return o
                return None
        """
        if not issubclass(object_type, DataObject):
            raise ValueError("Not a subclass of ovito.data.DataObject: {}".format(object_type))
        for obj in self.objects:
            if isinstance(obj, object_type):
                return obj
        return None

    def expect(self, object_type):
        """
        Looks up the first data object in this collection of the given class type.
        Raises a ``KeyError`` if there is no instance matching the type. Use :py:meth:`.find` instead
        to test if the data collection contains the given type of data object.

        :param object_type: The :py:class:`DataObject` subclass specifying the type of object to find.
        :return: The first instance of the given class or its subclasses from the :py:attr:`.objects` list.
        """
        o = self.find(object_type)
        if o is None: raise KeyError('Data collection does not contain a {}'.format(object_type))
        return o

    def find_all(self, object_type):
        """
        Looks up all data objects from this collection of the given class type.

        :param object_type: The :py:class:`DataObject` subclass that should be looked up.
        :return: A Python list containing all instances of the given class or its subclasses from the :py:attr:`.objects` list.

        Method implementation::

            def find_all(self, object_type):
                return [o for o in self.objects if isinstance(o, object_type)]
        """
        if not issubclass(object_type, DataObject):
            raise ValueError("Not a subclass of ovito.data.DataObject: {}".format(object_type))
        return [obj for obj in self.objects if isinstance(obj, object_type)]

    def add(self, obj):
        # This method has been deprecated and is here only for backward compatibility with OVITO 2.9.0.
        # Use DataCollection.objects.append() instead.
        if not obj in self.objects:
            self.objects.append(obj)

    def remove(self, obj):
        # This method has been deprecated and is here only for backward compatibility with OVITO 2.9.0.
        # Use del DataCollection.objects[] instead.
        index = self.objects.index(obj)
        if index >= 0:
            del self.objects[index]

    def replace(self, oldobj, newobj):
        # This method has been deprecated and is here only for backward compatibility with OVITO 2.9.0.
        index = self.objects.index(oldobj)
        if index >= 0:
            self.objects[index] = newobj
 
    @property
    def cell(self):
        # Here for backward compatibility with OVITO 2.9.0.
        # Returns the :py:class:`SimulationCell` stored in this data collection (if any).
        return self.find(SimulationCell)
