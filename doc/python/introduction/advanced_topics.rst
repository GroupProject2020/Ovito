==================================
Advanced topics
==================================

   * :ref:`saving_loading_pipelines`

.. _saving_loading_pipelines:

----------------------------------
Saving and loading pipelines
----------------------------------

The :py:func:`ovito.io.export_file` function lets you to save the output data of a pipeline to disk. But how do 
you save the definition of the pipeline itself, including the modifiers, to a file? The current version of OVITO can
save the entire *scene* to a .ovito state file using the :py:meth:`Scene.save() <ovito.Scene.save>` method. 
Thus, you need to make the :py:class:`~ovito.pipeline.Pipeline` part of the scene first using its :py:meth:`~ovito.pipeline.Pipeline.add_to_scene`
method:
   
.. literalinclude:: ../example_snippets/saving_pipeline.py
   :lines: -9

Unfortunately, there is no corresponding Python function for loading a scene from a .ovito state file. The only way
to restore the state is to :py:ref:`preload the .ovito file <preloading_program_state>` when executing a batch script 
with the :program:`ovitos` script interpreter using the :command:`-o` command line option:

.. code-block:: shell-session

	ovitos -o mypipeline.ovito script.py

The script :file:`script.py` will now execute in a context where the :py:class:`~ovito.Scene` was already initialized 
with the data from the .ovito state file. Instead of setting up a completely new pipeline, the script can therefore
work with the existing pipeline that was restored from the state file:

.. literalinclude:: ../example_snippets/saving_pipeline.py
   :lines: 13-

