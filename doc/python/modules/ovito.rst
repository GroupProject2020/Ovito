==================================
``ovito``
==================================

.. automodule:: ovito
   :members:
   :imported-members:

   .. py:data:: scene
      
      This module-level variable points to the global :py:class:`~ovito.Scene` object,
      which serves as context for all operations performed by the script. The :py:class:`~ovito.Scene` object
      represents the program state and provides access to the contents of the visualization scene::

         import ovito
         
         # Retrieve the output data of the pipeline that is currently selected in OVITO:
         data = ovito.scene.selected_pipeline.compute()

   .. py:data:: version
      
      This module-level attribute reports the OVITO program version number (as a tuple of three ints).

   .. py:data:: version_string
      
      This module-level attribute reports the OVITO program version (as a string).
      