.. _version_changes:

===================================
API version changes
===================================

This page documents changes to the Python programming interface introduced with new OVITO program releases.
It is supposed to help script authors adapt their scripts to the most recent version of the Python API.

------------------------------------
Migrating from OVITO 2.x to 3.0
------------------------------------

Release 3 of OVITO introduces significant interface changes. Various classes and methods
have been deprecated and replaced with new facilities. The old interfaces of OVITO 2.x no longer show up in the Python reference documentation,
however, a backward compatibility layer has been put in place to support execution of old scripts. 
Thus, in many cases, old Python scripts written for OVITO 2.x should still work, at least for now, but it is recommended
to update them in order to use the new programming interfaces described in the following.
The backward compatibility layer will be removed in a future version of OVITO.

Data pipelines and data sources
------------------------------------

The :py:class:`!ovito.ObjectNode` class has been renamed to :py:class:`~ovito.pipeline.Pipeline` and
moved to the new :py:mod:`ovito.pipeline` module. The old class name reflected the fact that instances
are part of the scene graph in OVITO. However, this aspect is less important from 
the Python script perspective and therefore :py:class:`~ovito.pipeline.Pipeline` is the more natural naming choice.

The :py:class:`~ovito.pipeline.StaticSource` is a type of data source for a :py:class:`~ovito.pipeline.Pipeline` 
and can hold a set of data objects that should be processed by the pipeline. The :py:class:`~ovito.pipeline.FileSource`
class maintains its role as the main data source type for pipelines, reading the input data from an external file.

The :py:attr:`!ObjectNode.output` field has been removed. It used to provide access to the cached results of the data pipeline 
after a call to :py:meth:`!ObjectNode.compute`. Now the computation results should be stored in a local variable instead::

   pipeline = import_file('simulation.dump')
   ...
   data = pipeline.compute()

:py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` returns a new :py:class:`~ovito.data.DataCollection` containing
the output data of the pipeline.

The ``DataCollection`` class
----------------------------------------

The :py:class:`~ovito.data.DataCollection` class no longer implements a dictionary interface to provide access to the contained data objects.
Instead, the new :py:attr:`~ovito.data.DataCollection.objects` field exposes all objects as an unordered list. 
The :py:meth:`!add`, :py:meth:`!remove` and :py:meth:`!replace` methods have been deprecated. 
Instead, you can insert/remove data objects as follows::

    cell = SimulationCell()
    data.objects.append(cell)
    data.objects.remove(cell)

The :py:class:`~ovito.data.DataCollection` properties :py:attr:`!.cell`, :py:attr:`!.surface` and :py:attr:`!.dislocations` have been deprecated.
Instead, the new general methods :py:meth:`~ovito.data.DataCollection.find` and :py:meth:`~ovito.data.DataCollection.expect`
should be used instead to retrieve data objects based on their type, e.g.::

    cell = data.expect(SimulationCell)

The properties :py:attr:`!.number_of_particles`, :py:attr:`!.number_of_half_bonds` and :py:attr:`!.number_of_full_bonds` have 
been deprecated. Instead, these numbers are now reported by the following properties::

    num_particles = data.particles.count
    num_bonds = data.bonds.count

The :py:meth:`!create_particle_property` and :py:meth:`!create_user_particle_property` methods for creating new particle and bond properties have 
been replaced by the :py:meth:`~ovito.data.ParticlesView.create_property` method in the new :py:class:`~ovito.data.ParticlesView` helper class, which 
is returned by the :py:attr:`DataCollection.particles <ovito.data.DataCollection.particles>` attribute.
Similarly, the :py:meth:`!create_bond_property` and :py:meth:`!create_user_bond_property` methods have 
been replaced by the :py:meth:`~ovito.data.BondsView.create_property` method in the new :py:class:`~ovito.data.BondsView` helper class, which 
is returned by the :py:attr:`DataCollection.bonds <ovito.data.DataCollection.bonds>` attribute.

Particle and bond properties
----------------------------------------

The :py:class:`~ovito.data.ParticleProperty` and :py:class:`~ovito.data.BondProperty` classes now inherit from a common base class,
:py:class:`~ovito.data.Property`, which provides the functionality common to all property types in OVITO.

Access to *standard* particle and bond properties via Python named attributes has been deprecated. Instead, they 
should be looked up by name, similar to *user-defined* properties::

    data = pipeline.compute()
    pos_property = data.particles.position     # <-- Deprecated
    pos_property = data.particles['Position']  # <-- Correct

Note that the :py:attr:`DataCollection.particles <ovito.data.DataCollection.particles>` object
behaves like a (read-only) dictionary of particle properties, providing a filtered view of the data :py:attr:`~ovito.data.DataCollection.objects` list of the :py:class:`~ovito.data.DataCollection`.

The :py:attr:`!array` and :py:attr:`!marray` accessor attributes of the :py:class:`~ovito.data.ParticleProperty` and :py:class:`~ovito.data.BondProperty`
classes have been deprecated. Instead, these classes themselves now behave like Numpy arrays::

    pos_property = data.particles['Position']
    print('Number of particles:', len(pos_property))
    print('Position of first particle:', pos_property[0])

However, note that :py:class:`~ovito.data.ParticleProperty` and :py:class:`~ovito.data.BondProperty` are not true Numpy array subclasses; they just mimic the Numpy array
interface to some extent. You can turn them into true Numpy arrays if needed in two ways::

    pos_array = numpy.asarray(pos_property)
    pos_array = pos_property[...]

In both cases no data copy is made. The Numpy array will be a view of the internal memory of the :py:class:`~ovito.data.Property`.
To modify the data stored in a :py:class:`~ovito.data.Property`, write access must be explicitly requested using a Python ``with`` 
statement::

    with pos_property:
        pos_property[0] = (0,0,0)

The old :py:attr:`!.marray` accessor attribute and a 
call to the deprecated :py:meth:`!ParticleProperty.changed` method to finalize the write transaction are no longer needed.

Simulation cells
------------------------------------------

The :py:class:`~ovito.data.SimulationCell` class now behaves like a read-only Numpy array of shape (3,4), providing direct
access to the cell vectors and the cell origin. The old :py:attr:`!.array` and :py:attr:`!.marray` accessor attributes have been deprecated.
Write access to the cell matrix now requires a ``with`` statement::

    cell = pipeline.source.cell
    with cell:
        cell[:,1] *= 1.1   # Expand cell along y-direction by scaling second cell vector

Bonds
------------------------------------------

OVITO 3.x no longer works with a half-bond representation. Older program versions represented each full bond A<-->B
as two separate half-bonds A-->B and B-->A. Now, only a single record per bond is maintained by OVITO.

The :py:class:`!ovito.data.Bonds` class has been removed. Instead, bond topology is now stored as a standard 
:py:class:`~ovito.data.BondProperty` named ``Topology``, which is accessible through the :py:class:`~ovito.data.BondsView` 
object. 

The :py:class:`!Bonds.Enumerator` helper class has been renamed to :py:class:`~ovito.data.BondsEnumerator`
and its constructor now expects a :py:class:`~ovito.data.DataCollection` instead of a :py:class:`!Bonds` object.

File I/O
------------------------------------

The :py:func:`ovito.io.import_file` function no longer requires the ``multiple_frames`` flag to load simulation files
containing more than one frame. This happens automatically now. Furthermore, :py:func:`~ovito.io.import_file` now 
supports loading file sequences that are specified as an explicit list of file paths. This makes it possible to 
load sets of files that are distributed over more than directory as one animation sequence.

The :py:func:`ovito.io.export_file` function now accepts not only a :py:class:`~ovito.pipeline.Pipeline` object which 
generates the data to be exported, but alternatively also any :py:class:`~ovito.data.DataCollection` or individual 
data objects.

Some of the file format names accepted by :py:func:`~ovito.io.export_file` have been renamed and the new ``vtk/trimesh`` 
has been added, which allows to export a :py:class:`~ovito.data.SurfaceMesh` to a VTK geometry file.

The :py:attr:`!FileSource.loaded_file` attribute has been removed. The path of the input data file is now accessible as an attribute
of the :py:class:`~ovito.data.DataCollection` interface, e.g.::

    pipeline = import_file('input.dump')
    data = pipeline.compute()
    print(data.attributes['SourceFile'])
    print(pipeline.source.attributes['SourceFile'])

The old :py:meth:`!DataCollection.to_ase_atoms` and :py:meth:`!DataCollection.create_from_ase_atoms` methods
have been refactored into the new :py:mod:`ovito.io.ase` module and are now standalone functions named :py:func:`~ovito.io.ase.ovito_to_ase` 
and :py:func:`~ovito.io.ase.ase_to_ovito`. The latter requires that the caller provides an existing data collection object
as destination for the atoms data, e.g. a :py:class:`~ovito.pipeline.StaticSource` instance.

Changes to the global ``DataSet`` class
------------------------------------------

The :py:attr:`!ovito.DataSet` class has been renamed to :py:class:`ovito.Scene` and the singleton class instance is now 
accessible as global variable :py:data:`ovito.scene`.

The :py:attr:`!DataSet.selected_node` and :py:attr:`!DataSet.scene_nodes` fields have been renamed to
:py:attr:`Scene.selected_pipeline <ovito.Scene.selected_pipeline>` and :py:attr:`Scene.pipelines <ovito.Scene.pipelines>` respectively.

The :py:class:`!AnimationSettings` class and the :py:attr:`!DataSet.anim` attribute have been deprecated.
Because of this, scripts no longer have control over the current time slider position. To determine the number of 
loaded animation frames, use the :py:attr:`FileSource.num_frames <ovito.pipeline.FileSource.num_frames>` attribute instead.

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

Changes to rendering functions
------------------------------------------

The :py:class:`!RenderSettings` class and the :py:meth:`Viewport.render` method have been deprecated. 
Instead, the :py:class:`~ovito.vis.Viewport` class now supports the new :py:meth:`~ovito.vis.Viewport.render_image`
and :py:meth:`~ovito.vis.Viewport.render_anim` methods, which directly accept the required render settings as keyword function 
parameters. 

Changes to the PythonViewportOverlay class
------------------------------------------

The signature of user-defined overlay functions has been changed. The :py:class:`~ovito.vis.PythonViewportOverlay` 
now passes a single parameter of the :py:class:`PythonViewportOverlay.Arguments <ovito.vis.PythonViewportOverlay.Arguments>`
type to the user function, which contains all necessary information. This helper class also provides additional utility methods for
projecting points from 3d space to 2d screen space, which may be used by the user-defined overlay function.

Changes to display objects
------------------------------------------

The :py:class:`!Display` base class has been renamed to :py:class:`~ovito.vis.DataVis`. Instead of *display objects*, the documentation now uses the term
*visual elements* when referring to the objects attached to some data objects, which are responsible for rendering 
the data. The :py:mod:`ovito.vis` module provides various visual element types, each derived from the common :py:class:`~ovito.vis.DataVis`
base class.

The :py:class:`~ovito.pipeline.Pipeline` class now provides the new :py:meth:`~ovito.pipeline.Pipeline.get_vis` method, which 
provides easier access to the visual elements leaving the data pipeline. Instead of accessing a visual element via the :py:attr:`~ovito.data.DataObject.vis` 
attribute of the data object it is attached to, the visual element can now directly be looked up based on its class type.

SurfaceMesh data object
------------------------------------------

The :py:class:`~ovito.data.SurfaceMesh` class has been greatly extended. It now provides access to
the periodic :py:attr:`~ovito.data.SurfaceMesh.domain` the surface mesh is embedded in as well as the vertices and faces
of the mesh. Export of the triangle mesh to a VTK file is now performed using the standard :py:func:`ovito.io.export_file`
function (``'vtk/trimesh'`` output format). 

Furthermore, the :py:class:`~ovito.data.SurfaceMesh` class now provides the :py:meth:`~ovito.data.SurfaceMesh.locate_point` method,
which can be used to determine whether a spatial point is located on the surface manifold, inside the region enclosed by the surface, or outside. 
