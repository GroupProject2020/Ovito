try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections
import sys
    
# Load the native modules.
from ..plugins.PyScript import Pipeline, PipelineStatus, ModifierApplication, Modifier, DataVis

# Implementation of the Pipeline.modifiers collection. 
def _Pipeline_modifiers(self):
    """ The sequence of modifiers in the pipeline.
    
        This list contains any modifiers that are applied to the input data provided by the pipeline's data :py:attr:`.source`. You
        can add and remove modifiers as needed using standard Python ``append()`` and ``del`` operations. The 
        head of the list represents the beginning of the pipeline, i.e. the first modifier receives the data from the 
        data :py:attr:`.source`, manipulates it and passes the results on to the second modifier in the list and so forth.
       
        Example: Adding a new modifier to the end of a data pipeline::
       
           pipeline.modifiers.append(WrapPeriodicImagesModifier())
    """    
    
    class PipelineModifierList(collections.MutableSequence):
        """ This is a helper class is used for the implementation of the Pipeline.modifiers field. It emulates a 
            mutable list of modifiers. The array is generated from the chain (linked list) of ModifierApplication instances
            that make up the pipeline.
        """
        def __init__(self, pipeline): 
            """ The constructor stores away a back-pointer to the owning Pipeline. """
            self.pipeline = pipeline

        def _modifierList(self):
            """ Builds an array with all modifiers in the pipeline by traversing the chain of ModifierApplications. """
            mods = []
            obj = self.pipeline.data_provider
            while isinstance(obj, ModifierApplication):
                mods.insert(0, obj.modifier)
                obj = obj.input
            return mods

        def _modAppList(self):
            """ Builds an array with all modifier application in the pipeline. """
            modapps = []
            obj = self.pipeline.data_provider
            while isinstance(obj, ModifierApplication):
                modapps.insert(0, obj)
                obj = obj.input
            return modapps

        def __len__(self):
            """ Determines the total number of modifiers in the pipeline. """
            count = 0
            obj = self.pipeline.data_provider
            while isinstance(obj, ModifierApplication):
                count += 1
                obj = obj.input
            return count

        def __iter__(self):
            """ Returns an iterator that visits all modifiers in the pipeline. """
            return self._modifierList().__iter__()
        
        def __getitem__(self, i):
            """ Return a modifier from the pipeline by index. """
            return self._modifierList()[i]

        def __setitem__(self, index, newMod):
            """ Replaces an existing modifier in the pipeline with a new one. """
            if not isinstance(newMod, Modifier):
                raise TypeError("Expected a modifier")
            if isinstance(index, slice):
                raise TypeError("This sequence type does not support slicing.")
            if not isinstance(index, int):
                raise TypeError("Expected integer index")
            if index < 0: index += len(self)
            modapplist = self._modAppList()
            modapp = newMod.create_modifier_application()
            assert(isinstance(modapp, ModifierApplication))
            modapp.modifier = newMod
            if index == len(modapplist) - 1 and index >= 0:
                assert(self.pipeline.data_provider == modapplist[-1])
                self.pipeline.data_provider = modapp
                modapp.input = modapplist[-1].input
            elif index < len(modapplist) - 1 and index >= 0:
                modapp.input = modapplist[index].input
                modapplist[index + 1].input = modapp
            else:
                raise IndexError("List index is out of range.")
            newMod.initialize_modifier(modapp)

        def __delitem__(self, index):
            """ Removes a modifier from the pipeline by index. """
            if isinstance(index, slice):
                raise TypeError("This sequence type does not support slicing.")
            if not isinstance(index, int):
                raise TypeError("Expected integer index")
            if index < 0: index += len(self)
            modapplist = self._modAppList()
            if index == len(modapplist) - 1 and index >= 0:
                assert(self.pipeline.data_provider == modapplist[-1])
                self.pipeline.data_provider = modapplist[-1].input
            elif index < len(modapplist) - 1 and index >= 0:
                modapplist[index + 1].input = modapplist[index].input
            else:
                raise IndexError("List index is out of range.")

        def insert(self, index, mod):
            """ Inserts a new modifier into the pipeline at a given position. """ 
            if not isinstance(mod, Modifier):
                raise TypeError("Expected a modifier")
            if not isinstance(index, int):
                raise TypeError("Expected integer index")
            if index < 0: index += len(self)
            modapplist = self._modAppList()
            modapp = mod.create_modifier_application()
            assert(isinstance(modapp, ModifierApplication))
            modapp.modifier = mod
            if index == len(modapplist) and index >= 0: 
                assert(self.pipeline.data_provider == modapplist[-1])
                self.pipeline.data_provider = modapp
                modapp.input = modapplist[-1]
            elif index <= len(modapplist) - 1 and index >= 0:
                modapp.input = modapplist[index].input
                modapplist[index].input = modapp
            else:
                raise IndexError("List index is out of range.")
            mod.initialize_modifier(modapp)

        def append(self, mod):
            """ Inserts a new modifier at the end of the pipeline. """ 
            if not isinstance(mod, Modifier):
                raise TypeError("Expected a modifier")
            modapp = mod.create_modifier_application()
            assert(isinstance(modapp, ModifierApplication))
            modapp.modifier = mod
            modapp.input = self.pipeline.data_provider
            self.pipeline.data_provider = modapp
            mod.initialize_modifier(modapp)

        def __str__(self):
            return str(self._modifierList())

    return PipelineModifierList(self)
Pipeline.modifiers = property(_Pipeline_modifiers)

def _Pipeline_compute(self, frame = None):
    """ Computes and returns the results of this data pipeline.

        This method requests an evaluation of the pipeline and blocks until the input data has been obtained from the
        data :py:attr:`.source` and all modifiers have been applied to the data. When invoking the :py:meth:`!compute` method repeatedly
        without changing the pipeline between calls, it may return immediately with cached results from an earlier evaluation.
        
        The optional *frame* parameter determines at which animation time the pipeline is evaluated. Animation frame numbering starts at 0. 
        If no frame is specified, the current animation position is used (frame 0 by default).

        The :py:meth:`!compute` method raises a ``RuntimeError`` when the pipeline could not be successfully evaluated for some reason.
        This can happen due to invalid modifier parameters or input file parsing errors, for example.

        Note: The returned :py:class:`~ovito.data.DataCollection` contains data objects that are owned by the pipeline.
        That means it is not safe to modify these objects as this would lead to unexpected side effects. 
        You can however use the :py:meth:`DataCollection.copy_if_needed() <ovito.data.DataCollection.copy_if_needed>` method
        to make a copy of data objects that you want to modify in place. See the :py:class:`~ovito.data.DataCollection` class
        for more information.

        :param int frame: The animation frame number at which the pipeline should be evaluated. 
        :returns: The :py:class:`~ovito.data.DataCollection` produced by the data pipeline.
    """
    if frame is not None:
        time = self.dataset.anim.frame_to_time(frame)
    else:
        time = self.dataset.anim.time

    state = self.evaluate_pipeline(time)
    if state.status.type == PipelineStatus.Type.Error:
        raise RuntimeError("Data pipeline evaluation failed: %s" % state.status.text)
    assert(state.status.type != PipelineStatus.Type.Pending)

    # Wait for worker threads to finish.
    # This is to avoid warning messages 'QThreadStorage: Thread exited after QThreadStorage destroyed'
    # during Python program exit.
    if not hasattr(sys, '__OVITO_BUILD_MONOLITHIC'):
        import PyQt5.QtCore
        PyQt5.QtCore.QThreadPool.globalInstance().waitForDone(0)

    return state.mutable_data
Pipeline.compute = _Pipeline_compute

# Provides access to the last results computed by the pipeline.
# This is attribute for backward compatibility with OVITO 2.9.0
Pipeline.output = property(lambda self: self.compute())

# Implementation of the Pipeline.get_vis() method:
def _Pipeline_get_vis(self, vis_type):
    """
    Looks up a visual element of a specified type in this pipeline's output.
    This method implicitly calls :py:meth:`.compute` and visits all data objects in the 
    pipeline's output until it finds one that has a visual element of the given class type attached. 

    :param vis_type: The :py:class:`~ovito.vis.DataVis` subclass to be looked up.
    :return: An instance of the given :py:class:`~ovit.vis.DataVis`-derived class; or ``None`` if there is no such visual element in this pipeline's output.

    The method can be used to easily access particular visual elements. The following code example demonstrates how the :py:class:`~ovito.vis.ParticlesVis`
    element that is attached to the ``Position`` particle property can be looked up:

    .. literalinclude:: ../example_snippets/pipeline_get_vis.py
       :lines: 7-14

    See the :py:mod:`ovito.vis` module for a list of visual element types in OVITO that can be looked up using this method.

    """
    if not issubclass(vis_type, DataVis):
        raise ValueError("Not a subclass of ovito.vis.DataVis: {}".format(vis_type))
    self.compute()
    for vis in self.vis_elements:
        if isinstance(vis, vis_type):
            return vis
    return None
Pipeline.get_vis = _Pipeline_get_vis

def _Pipeline_remove_from_scene(self):
    """ Removes the visual representation of the pipeline from the scene by deleting it from the :py:attr:`ovito.Scene.pipelines` list.
        The output data of the pipeline will disappear from the viewports.
    """
    # Remove pipeline from its parent's list of children
    if self.parent_node is not None:
        del self.parent_node.children[self.parent_node.children.index(self)]
    
    # Automatically unselect pipeline
    if self is self.dataset.selected_pipeline:
        self.dataset.selected_pipeline = None
Pipeline.remove_from_scene = _Pipeline_remove_from_scene

def _Pipeline_add_to_scene(self):
    """ Inserts the pipeline into the three-dimensional scene by appending it to the :py:attr:`ovito.Scene.pipelines` list.
        The visual representation of the computed data will appear in rendered images and in the interactive viewports of the 
        graphical OVITO version.
        
        You can remove the pipeline from the scene again using :py:meth:`.remove_from_scene`.
    """
    if not self in self.dataset.pipelines:
        self.dataset.pipelines.append(self)
    
    # Select piepline
    self.dataset.selected_pipeline = self
Pipeline.add_to_scene = _Pipeline_add_to_scene
