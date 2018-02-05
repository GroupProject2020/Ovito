.. _modifiers_overview:

===================================
Data pipelines & modifiers
===================================

The following introduction assumes that you have already read the :py:ref:`scripting_api_overview` page.

Modifiers are composable function objects that form a `data processing pipeline <https://en.wikipedia.org/wiki/Pipeline_(software)>`_.
They dynamically modify, filter, analyze or extend the data that flows down the pipeline. Here, with *data* we mean
any form of information that OVITO can work with, e.g. particles and their properties, bonds, the simulation cell,
triangles meshes, voxel data, etc. The main purpose of the pipeline concept is to enable a non-destructive and repeatable workflow, i.e.,
instead of having a single dataset that is being modified in-place, a modification pipeline --once set up-- can be used 
repeatedly on multiple input data. And since the result of a pipeline is computed dynamically from the original input data, it is possible to 
change the pipeline at any time and revert actions of individual modifiers, for example. 

A processing pipeline is represented by an instance of the :py:class:`~ovito.pipeline.Pipeline` class in OVITO.
Initially, a pipeline contains no modifiers. That means its output will be identical to its input. The input data
is provided by a separate source object that is attached to the pipeline. 
Typically, this :py:attr:`~ovito.pipeline.Pipeline.source` object is an instance of the :py:class:`~ovito.pipeline.FileSource` class, which reads the input data
from some external data file.

You can insert a modifier into a :py:class:`~ovito.pipeline.Pipeline` by creating a new 
instance of the corresponding modifier class (see the :py:mod:`ovito.modifiers` module for all available modifier types) and then 
adding it to the pipeline's :py:attr:`~ovito.pipeline.Pipeline.modifiers` list::

   from ovito.modifiers import AssignColorModifier

   modifier = AssignColorModifier( color=(0.5, 1.0, 0.0) )
   pipeline.modifiers.append(modifier)
   
The modifiers in the :py:attr:`Pipeline.modifiers <ovito.pipeline.Pipeline.modifiers>` list are executed from front to back, i.e.,
appending a modifier to the list positions it at the end of the data pipeline and it will be the last one to process
the data that is flowing down the pipeline. In other words, it will see the effect of all preceding modifiers in the sequence.

.. image:: graphics/Pipeline.*
   :width: 50 %
   :align: center

Note that inserting a new modifier into the pipeline --like any change to a pipeline or its modifiers-- does not 
immediately trigger a computation. The modifier's effect will be computed only when the results of the pipeline are requested. 
Evaluation of the pipeline can be triggered either implicitly, e.g. when

  * rendering an image or movie,
  * updating the interactive viewports in OVITO's graphical user interface, 
  * or exporting data using the :py:func:`ovito.io.export_file` function.
  
But you can also explicitly request an evaluation of the pipeline by calling its :py:meth:`~ovito.pipeline.Pipeline.compute` method.
This method returns a new :py:class:`~ovito.data.PipelineFlowState` object, which is a form of :py:class:`~ovito.data.DataCollection` 
holding the set of data objects that left the pipeline::

    >>> data = pipeline.compute()
    >>> print(data.objects)
    [SimulationCell(), ParticleProperty('Position'), ParticleProperty('Color')]

In this example, the output data collection contains three data objects: a :py:class:`~ovito.data.SimulationCell`
object and two :py:class:`~ovito.data.ParticleProperty` objects, which store the particle positions and 
particle colors, respectively. We will learn more about the :py:class:`~ovito.data.DataCollection` class and
the representation of data later.

Note that it is possible to change parameters of existing modifiers in a pipeline. Again, this does not immediately trigger 
a recomputation of the pipeline (unlike in the graphical user interface, where changing a modifier's parameters 
lets OVITO immediately recompute the results and update the interactive viewports). For example, we can 
produce two alternative computation results by first evaluating the pipeline, then changing one of the modifiers, and then 
evaluating the pipeline a second time::

    >>> pipeline = import_file("simulation.dump")
    >>> pipeline.modifiers.append(AssignColorModifier(color = (0.5, 1.0, 0.0)))
    
    >>> data_A = pipeline.compute()
    >>> pipeline.modifiers[0].color = (0.8, 0.8, 1.0)
    >>> data_B = pipeline.compute()

    >>> data_A.particle_properties['Color'][...]
    array([[ 0.5,  1. ,  0. ],
           [ 0.5,  1. ,  0. ],
            ..., 
           [ 0.5,  1. ,  0. ],
           [ 0.5,  1. ,  0. ]])

    >>> data_B.particle_properties['Color'][...]
    array([[ 0.8,  0.8,  1. ],
           [ 0.8,  0.8,  1. ],
            ..., 
           [ 0.8,  0.8,  1. ],
           [ 0.8,  0.8,  1. ]])

--------------------------------------------------------------
Processing of time-dependent data and simulation trajectories
--------------------------------------------------------------

As discussed in the :ref:`file_io_overview` section, it is possible to import simulation trajectories into 
OVITO consisting of a sequence of frames. The :py:class:`~ovito.pipeline.FileSource` object providing
the input data for a :py:class:`~ovito.pipeline.Pipeline` will feed one frame at a time to the pipeline in this case.
The pipeline never processes all frames in a trajectory at once; you rather request the processing
of a specific simulation frame by passing a *time* argument to the pipeline's :py:meth:`~ovito.pipeline.Pipeline.compute`
method, e.g.::

    >>> pipeline = import_file("trajectory_*.dump")
    >>> data0 = pipeline.compute(0)
    >>> data1 = pipeline.compute(1)

The *time* argument specifies the animation frame number at which the pipeline should be evaluated, with 0 denoting the first frame in the loaded sequence.
Typically, a ``for``-loop of the following form is used to iterate over all frames of a simulation sequence and process them one by one::

    for frame in range(pipeline.source.num_frames):
        data = pipeline.compute(frame)
        ...

Here, we accessed the :py:attr:`FileSource.num_frames <ovito.pipeline.FileSource.num_frames>` property to determine how many
frames the input trajectory contains.

Keep in mind that a :py:class:`~ovito.pipeline.Pipeline` is a reusable object, which normally should be set up only once and 
then used many times to process multiple frames or input files. Thus, adding modifiers to the pipeline *inside* the for-loop is 
wrong::

    # WRONG (!!!):
    for frame in range(pipeline.source.num_frames):
        pipeline.modifiers.append(AtomicStrainModifier(cutoff = 3.2))
        data = pipeline.compute(frame)
        ...

Note how this loop would keep appending additional modifiers to the same pipeline, making it longer and longer with every iteration.
As a result, the computation of atomic strain values would be performed over and over again for the same data
when :py:meth:`~ovito.pipeline.Pipeline.compute` is called. 
Instead, the addition of the modifier should be performed exactly once *before* entering the loop::

    # CORRECT:

    # 1st step: pipeline setup
    pipeline.modifiers.append(AtomicStrainModifier(cutoff = 3.2))

    # 2nd step: pipeline evaluation
    for frame in range(pipeline.source.num_frames):
        data = pipeline.compute(frame)
        ...

---------------------------------
Composing modifiers
---------------------------------

To be written...

---------------------------------
Analysis modifiers
---------------------------------

Analysis modifiers perform some computation based on the data they receive from the upstream part of the
modification pipeline (or the :py:class:`~ovito.io.FileSource`). Typically they produce new 
output data (for example an additional particle property), which is fed back into the pipeline 
where it will be accessible to the following modifiers (e.g. a :py:class:`~ovito.modifiers.ColorCodingModifier`).

Let us take the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` as an example for a typical analysis modifier. 
It takes the particle positions as input and classifies each particle as either FCC, HCP, BCC, or some other
structural type. This per-particle information computed by the modifier is inserted into the pipeline as a new 
:py:class:`~ovito.data.ParticleProperty` data object. Since it flows down the pipeline, this particle property
is accessible by subsequent modifiers and will eventually arrive in the node's output data collection
where we can access it from a Python script::

    >>> cna = CommonNeighborAnalysis()
    >>> node.modifiers.append(cna)
    >>> node.compute()
    >>> print(node.output.particle_properties.structure_type.array)
    [1 0 0 ..., 1 2 0]
    
Note that the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` encodes the computed
structural type of each particle as an integer number (0=OTHER, 1=FCC, ...). 

In addition to this kind of per-particle data, many analysis modifiers generate global information
as part of their computation. This information, which typically consists of scalar quantities, is inserted into the data 
pipeline as *attributes*. For instance, the  :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` reports
the total number of particles that match the FCC structure type as an attribute named ``CommonNeighborAnalysis.counts.FCC``::

    >>> node.output.attributes['CommonNeighborAnalysis.counts.FCC']
    1262
    
Note how we could have obtained the same value by explicitly counting the number of particles of FCC type
ourselves::

    >>> structure_property = node.output.particle_properties.structure_type.array
    >>> numpy.count_nonzero(structure_property == CommonNeighborAnalysisModifier.Type.FCC)
    1262
    
Attributes are stored in the :py:attr:`~ovito.data.DataCollection.attributes` dictionary of the :py:class:`~ovito.data.DataCollection`.
The class documentation of each modifier lists the attributes that it generates.

---------------------------------
User-defined modifiers
---------------------------------

OVITO provides a large collection of built-in modifier types, which are all found in the :py:mod:`ovito.modifiers` module.
But it is also possible for you to write your own type of modifier in Python, which can participate in the pipeline system
just as the built-in modifiers. More on this advanced topic can be found in the :py:ref:`writing_custom_modifiers` section.
