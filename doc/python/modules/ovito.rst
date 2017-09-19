==================================
``ovito``
==================================

.. automodule:: ovito
   :members:
   :imported-members:

   .. py:data:: dataset
      
      This module-level attribute points to the current :py:class:`~ovito.DataSet`
      which serves as context for all operations performed by the script. The :py:class:`~ovito.DataSet`
      represents the program state and provides access to the viewports, the three-dimensional visualization scene, 
      and the current animation and render settings.

   .. py:data:: version
      
      This module-level attribute reports the OVITO program version number (as a tuple of three ints).

   .. py:data:: version_string
      
      This module-level attribute reports the OVITO program version (as a string).
      