.. _version_changes:

===================================
API version changes
===================================

This page documents changes to the Python programming interface introduced by new OVITO program releases.
It is supposed to help script authors adapt their scripts to the most recent version of the Python API.

------------------------------------
Migration from OVITO 2.x to 3.0
------------------------------------

This major program update introduces significant interface changes. Various classes and methods
have been deprecated and replaced with new facilities. The old interfaces no longer show up in the Python reference documentation,
however, a backward compatibility layer has been put in place to support execution of old scripts. 
Thus, in many cases old Python scripts written for OVITO 2.x should still work, at least for now, but it is recommended
to update them in order to use the new programming interfaces described in the following.
The backward compatibility layer will be removed in a future version of OVITO.

Data pipelines and data sources
------------------------------------

The :py:class:`!ovito.ObjectNode` class has been renamed to :py:class:`~ovito.pipeline.Pipeline` and
moved into the new :py:mod:`ovito.pipeline` module. The old class name reflected the fact that instances
are part of the scene graph in OVITO. However, this aspect is less important from 
the Python script perspective and therefore :py:class:`~ovito.pipeline.Pipeline` is a more natural 
choice.

:py:class:`~ovito.data.DataCollection` is now an abstract base class for the concrete classes
:py:class:`~ovito.pipeline.StaticSource`, :py:class:`~ovito.pipeline.FileSource` and :py:class:`~ovito.data.PipelineFlowState`. 
The :py:class:`~ovito.pipeline.StaticSource` is a type of data source for a :py:class:`~ovito.pipeline.Pipeline` 
and can hold a set of data objects that should be processed by the pipeline. The :py:class:`~ovito.pipeline.FileSource`
class maintains its role as a data source for a pipeline, reading the input data from an external file.
The :py:class:`~ovito.data.PipelineFlowState` is a type of data collection that is returned by the 
:py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline>` method and holds the results of a pipeline evaluation.

The :py:attr:`!ObjectNode.output` field has been deprecated. It used to provide access to the cached results of a data pipeline 
after a call to :py:meth:`!ObjectNode.compute`. Now the pipeline results should be stored in a local variable instead::

   pipeline = import_file('simulation.dump')
   ...
   data = pipeline.compute()
   print(data.particle_properties.keys())

In the example above, the variable ``data`` points to a :py:class:`~ovito.data.PipelineFlowState` returned by :py:meth:`~ovito.pipeline.Pipeline.compute`.

The ``DataCollection`` class
----------------------------------------

The :py:class:`~ovito.data.DataCollection` class no longer implements a dictionary interface to provide access to the contained data objects.
Instead, the new :py:attr:`~ovito.data.DataCollection.objects` field exposes all objects as an unordered list. 
The :py:meth:`!add`, :py:meth:`!remove` and :py:meth:`!replace` methods have been deprecated. 
Instead, you can insert/remove data objects as follows::

    cell = SimulationCell()
    data.objects.append(cell)
    data.objects.remove(cell)

The :py:class:`~ovito.data.DataCollection` properties :py:attr:`!.cell`, :py:attr:`!.bonds`, :py:attr:`!.surface` and :py:attr:`!.dislocations` have been deprecated.
Instead, the new general methods :py:meth:`~ovito.data.DataCollection.find` and :py:meth:`~ovito.data.DataCollection.expect`
should be used to retrieve particular data objects, e.g.::

    cell = data.expect(SimulationCell)
    bonds = data.expect(Bonds)

The properties :py:attr:`!.number_of_particles`, :py:attr:`!.number_of_half_bonds` and :py:attr:`!.number_of_full_bonds` have 
been deprecated. Instead, these numbers should be obtained from the length of the ``Position`` particle property and the
:py:class:`~ovito.data.Bonds` objects, respectively::

    num_particles = len(data.particle_properties['Position'])
    num_bonds = len(data.expect(Bonds))

The :py:meth:`!create_particle_property` and :py:meth:`!create_user_particle_property` methods have 
been replaced by the :py:meth:`~ovito.data.ParticlePropertiesView.create` method in the new :py:class:`~ovito.data.ParticlePropertiesView` helper class, which 
is returned by the :py:attr:`DataCollection.particle_properties <ovito.data.DataCollection.particle_properties>` attribute.
Similarily, the :py:meth:`!create_bond_property` and :py:meth:`!create_user_bond_property` methods have 
been replaced by the :py:meth:`~ovito.data.BondPropertiesView.create` method in the new :py:class:`~ovito.data.BondPropertiesView` helper class, which 
is returned by the :py:attr:`DataCollection.bond_properties <ovito.data.DataCollection.bond_properties>` attribute.

Particle and bond properties
----------------------------------------

The :py:class:`~ovito.data.ParticleProperty` and :py:class:`~ovito.data.BondProperty` classes now have a common base class,
:py:class:`~ovito.data.Property`, which provides the functionality common to all property types in OVITO.

Access to *standard* particle and bond properties via Python named attributes has been deprecated. Instead, they 
should be looked up by name, similar to *user-defined* properties::

    data = pipeline.compute()
    pos_property = data.particle_properties.position     # <-- Deprecated
    pos_property = data.particle_properties['Position']  # <-- Correct

Note that the :py:attr:`DataCollection.particle_properties <ovito.data.DataCollection.particle_properties>` object
behaves like a (read-only) dictionary of particle properties, providing a filtered view of the data :py:attr:`~ovito.data.DataCollection.objects` list in the :py:class:`~ovito.data.DataCollection`.

The :py:attr:`!array` and :py:attr:`!marray` accessor attributes of the :py:class:`~ovito.data.ParticleProperty` and :py:class:`~ovito.data.BondProperty`
classes have been deprecated. Instead, these classes themselves now behave like Numpy arrays::

    pos_property = data.particle_properties['Position']
    print('Number of particles:', len(pos_property))
    print('Position of first particle:', pos_property[0])

However, note that :py:class:`~ovito.data.ParticleProperty` and :py:class:`~ovito.data.BondProperty` are not true Numpy array subclasses; they just mimic the Numpy array
interface to some extent. You can turn them into true Numpy arrays if needed in two ways::

    pos_array = numpy.asarray(pos_property)
    pos_array = pos_property[...]

In both cases no data copy is made. The Numpy array will be a view of the internal memory of the :py:class:`~ovito.data.Property`.
To modify the data of a :py:class:`~ovito.data.Property`, write access must be explicitly requested using a Python ``with`` 
statement::

    with pos_property.modify() as pos_array:
        pos_array[0] = (0,0,0)

The :py:meth:`~ovito.data.Property.modify` method used in the ``with`` statement returns a temporary Numpy array that is writable and which allows
modifying the internal data of the :py:class:`~ovito.data.Property`. The old :py:attr:`!.marray` accessor and a 
call to the deprecated :py:meth:`!ParticleProperty.changed` method to finalize the write transaction are no longer needed.

Simulation cells
------------------------------------------

The :py:class:`~ovito.data.SimulationCell` class now behaves like a read-only Numpy array of shape (3,4), providing direct
access to the cell vectors and the cell origin. The old :py:attr:`!.array` and :py:attr:`!.marray` accessor attributes have been deprecated.
Write access to the cell matrix now requires a ``with`` statement::

    cell = pipeline.source.cell
    with cell.modify() as matrix:
        matrix[:,1] *= 1.1   # Expand cell along y-direction by scaling second cell vector

Bonds
------------------------------------------

OVITO 3.x no longer works with a half-bond representation. Older program versions represented each full bond A<-->B
as two separate half-bonds A-->B and B-->A in the :py:class:`~ovito.data.Bonds` array. Now only a single record is
stored per full bond. Accordingly, the array size of :py:class:`~ovito.data.BondProperty` objects has been cut in half as well. 

The :py:class:`~ovito.data.Bonds` class now mimics the Numpy array interface, giving direct access to the stored bonds list. 
The old :py:attr:`!Bonds.array` accessor attribute has been deprecated.

The :py:class:`!Bonds.Enumerator` helper class has been renamed to :py:class:`~ovito.data.BondsEnumerator`.

File I/O
------------------------------------

The :py:func:`ovito.io.export_file` function now accepts not only a :py:class:`~ovito.pipeline.Pipeline` object which 
generates the data to be exported, but alternatively also any :py:class:`~ovito.data.DataCollection` or individual 
data objects.

Some of the file format names accepted by :py:func:`~ovito.io.export_file` have been renamed and the new ``vtk/trimesh`` 
has been added, which allows to export a :py:class:`~ovito.data.SurfaceMesh` to a VTK geometry file.

The old :py:meth:`!DataCollection.to_ase_atoms` and :py:meth:`!DataCollection.create_from_ase_atoms` methods
have been refactored into the new :py:mod:`ovito.io.ase` module and are now standalone functions named :py:func:`~ovito.io.ase.ovito_to_ase` 
and :py:func:`~ovito.io.ase.ase_to_ovito`. The latter requires that the caller provides an existing data collection object
as destination for the atoms data, e.g. a :py:class:`~ovito.pipeline.StaticSource` instance.

Changes to the global ``DataSet`` class
------------------------------------------

The :py:attr:`!DataSet.selected_node` and :py:attr:`!DataSet.scene_nodes` fields have been renamed to
:py:attr:`DataSet.selected_pipeline <ovito.DataSet.selected_pipeline>` and :py:attr:`DataSet.scene_pipelines <ovito.DataSet.scene_pipelines>` respectively.

Changes to modifiers
------------------------------------------

The :py:class:`!SelectExpressionModifier` has been renamed to :py:class:`~ovito.modifiers.ExpressionSelectionModifier`.

The :py:class:`!DeleteSelectedParticlesModifier` has been renamed to :py:class:`~ovito.modifiers.DeleteSelectedModifier` and can now operate on
bonds too.

The :py:class:`!SelectParticleTypeModifier` has been renamed to :py:class:`~ovito.modifiers.SelectTypeModifier` and can now operate on
bonds too. Furthermore, it is now possible to specify the set of particle :py:attr:`~ovito.modifiers.SelectTypeModifier.types` to select
in terms of type *names*. Before, it was only possible to select particles based on *numeric* type IDs.

The following modifier classes have been generalized and gained a new :py:attr:`!operate_on` field that controls what kind(s) of data elements (e.g. particles,
bonds, surfaces, etc.) the modifier should act on:

   * :py:class:`~ovito.modifiers.AffineTransformationModifier`
   * :py:class:`~ovito.modifiers.AssignColorModifier` 
   * :py:class:`~ovito.modifiers.ClearSelectionModifier`
   * :py:class:`~ovito.modifiers.ColorCodingModifier` 
   * :py:class:`~ovito.modifiers.DeleteSelectedModifier` 
   * :py:class:`~ovito.modifiers.InvertSelectionModifier` 
   * :py:class:`~ovito.modifiers.HistogramModifier` 
   * :py:class:`~ovito.modifiers.SelectTypeModifier` 

SurfaceMesh data object
------------------------------------------

The :py:class:`~ovito.data.SurfaceMesh` class has been greatly extended. It now provides access to
the periodic :py:attr:`~ovito.data.SurfaceMesh.domain` the surface mesh is embedded in as well as the vertices and faces
of the mesh. Export of the triangle mesh to a VTK file is now performed using the standard :py:func:`ovito.io.export_file`
function (``'vtk/trimesh'`` output format). 

Furthermore, the :py:class:`~ovito.data.SurfaceMesh` class now provides the :py:meth:`~ovito.data.SurfaceMesh.locate_point` method,
which can be used to determine whether a spatial point is located on the surface manifold, inside the region enclosed by the surface, or outside. 
