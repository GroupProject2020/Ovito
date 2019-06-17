==================================
Advanced topics
==================================

This section covers several advanced topics related to OVITO's scripting interface:

   * :ref:`saving_loading_pipelines`

.. _saving_loading_pipelines:

----------------------------------
Saving and loading pipelines
----------------------------------

The :py:func:`ovito.io.export_file` function lets you to save the output data computed by a pipeline to disk. But how do
you save the definition of the pipeline itself, including all the modifiers, to a file? The current version of OVITO can
save the entire *scene* to a .ovito state file using the :py:meth:`Scene.save() <ovito.Scene.save>` method.
Thus, in order to save a :py:class:`~ovito.pipeline.Pipeline` you need to first make it part of the scene using its :py:meth:`~ovito.pipeline.Pipeline.add_to_scene`
method:

.. literalinclude:: ../example_snippets/saving_pipeline.py
   :lines: -9

Unfortunately, there currently exists no corresponding Python function for loading a scene back into memory from a .ovito state file.
The only way to restore the scene state is to :py:ref:`preload the .ovito file <preloading_program_state>` when executing a batch script.
This is done by using the :command:`-o` command line option of the :program:`ovitos` script interpreter:

.. code-block:: shell-session

	ovitos -o mypipeline.ovito script.py

The code in :file:`script.py` will now be executed in a context where the :py:class:`~ovito.Scene` was already initialized
with the state loaded from the .ovito scene file. Instead of setting up a completely new pipeline, the script can therefore
work with the existing pipeline that was restored from the state file:

.. literalinclude:: ../example_snippets/saving_pipeline.py
   :lines: 13-

