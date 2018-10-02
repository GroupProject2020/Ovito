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
moved to the new :py:mod:`ovito.pipeline` module. 

The :py:class:`~ovito.pipeline.StaticSource` is a special type of data source for a :py:class:`~ovito.pipeline.Pipeline` 
and can hold a set of data objects that should be processed by the pipeline. The :py:class:`~ovito.pipeline.FileSource`
class maintains its role as the main data source type for pipelines, reading the input data from an external file.

The :py:attr:`!ObjectNode.output` field has been removed. It used to provide access to the cached results of the data pipeline 
after a call to :py:meth:`!ObjectNode.compute`. Now the computation results should be requested using :py:meth:`~ovito.pipeline.Pipeline.compute` and stored in a local variable instead::

   pipeline = import_file('simulation.dump')
   ...
   data = pipeline.compute()

:py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` returns a new :py:class:`~ovito.data.DataCollection` containing
the output data of the pipeline after evaluating any modifiers that are currently part of the pipeline.

The ``DataCollection`` class
----------------------------------------

The new :py:attr:`~ovito.data.DataCollection.objects` field exposes all objects in a data collection as an unordered list. 
You can insert/remove data objects using standard Python list manipulation statements, e.g.::

    cell = SimulationCell()
    data.objects.append(cell)
    data.objects.remove(cell)

The properties :py:attr:`!.number_of_particles`, :py:attr:`!.number_of_half_bonds` and :py:attr:`!.number_of_full_bonds` have 
been deprecated. Instead, these numbers are now reported by the :py:attr:`~ovito.data.PropertyContainer.count` attribute 
of the :py:class:`~ovito.data.Particles` and the :py:class:`~ovito.data.Bonds` container objects::

    num_particles = data.particles.count
    num_bonds = data.particles.bonds.count


Particle and bond properties
----------------------------------------

The :py:class:`!ParticleProperty` and :py:class:`!BondProperty` classes have been replaced with the generic
:py:class:`~ovito.data.Property` class, which provides the functionality common to all property types in OVITO.
The :py:class:`~ovito.data.PropertyContainer` class has been introduced as a generic container type for 
:py:class:`~ovito.data.Property` objects. OVITO knows several specializations of this generic container type, 
e.g. :py:class:`~ovito.data.Particles`, :py:class:`~ovito.data.Bonds`, :py:class:`~ovito.data.VoxelGrid` and
:py:class:`~ovito.data.DataSeries`, that each represent different collections of elements. 

The :py:class:`ovito.data.Particles` container behaves like a (read-only) dictionary of particle properties, 
providing key-based access to the :py:attr:`~ovito.data.Property` objects it manages.

The :py:attr:`!ParticleProperty.array` and :py:attr:`!ParticleProperty.marray` attributes
for accessing property values have been deprecated. Instead, the :py:class:`~ovito.data.Property` object itself now behaves like a Numpy array::

    pos_property = data.particles['Position']
    assert(len(pos_property) == data.particles.count)
    print('XYZ position of first particle:', pos_property[0])

Note, however, that :py:class:`~ovito.data.Property` is not a true Numpy array subclass; it just mimics the Numpy array
interface to some extent. You can turn it into true Numpy array if needed in two ways::

    pos_array = numpy.asarray(pos_property)
    pos_array = pos_property[...]

In both cases no data copy is made. The Numpy array will be a view of the internal memory of the :py:class:`~ovito.data.Property`.
To modify the data stored in a :py:class:`~ovito.data.Property`, write access must be explicitly requested using a Python ``with`` 
statement::

    with pos_property:
        pos_property[0] = (0,0,0)

The old :py:attr:`!ParticleProperty.marray` accessor attribute and a 
call to the removed :py:meth:`!ParticleProperty.changed` method to finalize the write transaction are no longer needed.

Simulation cells
------------------------------------------

The :py:class:`~ovito.data.SimulationCell` class now behaves like a read-only Numpy array of shape (3,4), providing direct
access to the cell vectors and the cell origin. The old :py:attr:`!array` and :py:attr:`!marray` accessor attributes have been deprecated.
Write access to the cell matrix now requires a ``with`` statement::

    cell = data.cell_
    with cell:
        cell[:,1] *= 1.1   # Expand cell along y-direction by scaling second cell vector

Bonds
------------------------------------------

OVITO 3.x no longer works with a half-bond representation. Older program versions represented each full bond A<-->B
as two individual half-bonds A-->B and B-->A. Now, only a single record per bond is maintained by OVITO.

The :py:class:`!ovito.data.Bonds` container class stores the bond topology as a standard 
:py:class:`~ovito.data.Property` named ``Topology``, which is a *N* x 2 array of integer indices into the particles list::

    topology = data.particles.bonds['Topology']
    assert(topology.shape == (data.particles.bonds.count, 2))

The :py:class:`!Bonds.Enumerator` helper class has been renamed to :py:class:`~ovito.data.BondsEnumerator`.

File I/O
------------------------------------

The :py:func:`ovito.io.import_file` function no longer requires the ``multiple_frames`` flag to load simulation files
containing more than one frame. Detection of multi-timestep files happens automatically now. Furthermore, :py:func:`~ovito.io.import_file` now 
supports loading file sequences that are specified as an explicit list of file paths. This makes it possible to 
load sets of files that are distributed over everal directories as single animation sequence.

The :py:func:`ovito.io.export_file` function now accepts not only a :py:class:`~ovito.pipeline.Pipeline` object which 
generates the data to be exported, but alternatively also any :py:class:`~ovito.data.DataCollection` or individual 
data objects.

Some of the file format names accepted by :py:func:`~ovito.io.export_file` have been renamed and the new ``vtk/trimesh`` 
has been added, which allows you to export a :py:class:`~ovito.data.SurfaceMesh` to a VTK geometry file.

The :py:attr:`!FileSource.loaded_file` attribute has been removed. The path of the input data file is now accessible as an attribute
of the :py:class:`~ovito.data.DataCollection` interface, e.g.::

    pipeline = import_file('input.dump')
    data = pipeline.compute()
    print(data.attributes['SourceFile'])

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

Several modifier classes have been renamed in OVITO 3.0:

=========================================================== ===========================================================
Old modifier name:                                          New modifier name: 
=========================================================== ===========================================================
:py:class:`!SelectExpressionModifier`                       :py:class:`~ovito.modifiers.ExpressionSelectionModifier`
:py:class:`!DeleteSelectedParticlesModifier`                :py:class:`~ovito.modifiers.DeleteSelectedModifier`
:py:class:`!SelectParticleTypeModifier`                     :py:class:`~ovito.modifiers.SelectTypeModifier`
:py:class:`!CombineParticleSetsModifier`                    :py:class:`~ovito.modifiers.CombineDatasetsModifier`
:py:class:`!BinAndReduceModifier`                           :py:class:`~ovito.modifiers.SpatialBinningModifier`
=========================================================== ===========================================================

The following modifier classes have been generalized and gained a new :py:attr:`!operate_on` field that controls what kind(s) of data elements (e.g. particles,
bonds, voxel data, etc.) the modifier should act on:

   * :py:class:`~ovito.modifiers.AffineTransformationModifier`
   * :py:class:`~ovito.modifiers.AssignColorModifier` 
   * :py:class:`~ovito.modifiers.ClearSelectionModifier`
   * :py:class:`~ovito.modifiers.ColorCodingModifier` 
   * :py:class:`~ovito.modifiers.ComputePropertyModifier` 
   * :py:class:`~ovito.modifiers.DeleteSelectedModifier` 
   * :py:class:`~ovito.modifiers.ExpressionSelectionModifier` 
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
*visual elements*. The :py:mod:`ovito.vis` module provides various visual element types, each derived from the common :py:class:`~ovito.vis.DataVis`
base class.

SurfaceMesh data object
------------------------------------------

The :py:class:`~ovito.data.SurfaceMesh` class has been greatly extended. It now provides access to
the periodic :py:attr:`~ovito.data.SurfaceMesh.domain` the surface mesh is embedded in as well as the vertices and faces
of the mesh. Export of the triangle mesh to a VTK file is now performed using the standard :py:func:`ovito.io.export_file`
function (``'vtk/trimesh'`` output format). 

Furthermore, the :py:class:`~ovito.data.SurfaceMesh` class now provides the :py:meth:`~ovito.data.SurfaceMesh.locate_point` method,
which can be used to determine whether a spatial point is located on the surface manifold, inside the region enclosed by the surface, or outside. 
