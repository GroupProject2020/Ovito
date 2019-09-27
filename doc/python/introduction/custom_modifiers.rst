.. _writing_custom_modifiers:

===================================
User-defined modifiers
===================================

OVITO's Python programming interface allows you to write your own modifier functions that participate in the
:ovitoman:`data pipeline system <../../usage.modification_pipeline>` of OVITO. Writing your own modifier function
is useful in situations where the built-in modifiers of OVITO (found in the :py:mod:`ovito.modifiers` module) are not sufficient
to solve your specific problem at hand.

----------------------------------------------
Defining the modifier function
----------------------------------------------

You develop a user-defined modifier by simply writing a Python function, which will get automatically called by OVITO's
pipeline system whenever the results need to be computed. The Python function must have the following signature::

  def modify(frame, data):
      ...

The pipeline system will call your modifier function with two parameters: The current animation
frame number (``"frame"``) at which the pipeline is being evaluated and a :py:class:`~ovito.data.DataCollection` (``"data"``)
holding the information that is flowing down the pipeline and which the function should operate on.
Your modifier function should not return any value. If you want your function to manipulate the pipeline data in some way, it should do so
by modifying the :py:class:`~ovito.data.DataCollection` in-place.

You need to perform one of the following steps to insert your modifier function into the pipeline.

    -  If you are working within the graphical version of OVITO, you can integrate the Python function into the pipeline by
       inserting a :ovitoman:`Python script <../../particles.modifiers.python_script>` modifier with the pipeline editor.
       An integrated code editor allows you to directly type in the source code of the ``modify()`` Python function.

    -  If you want to use the modifier function within a :py:ref:`batch script <scripting_running>`, the script should
       define and insert the modifier function into the pipeline as follows::

            def my_mod_function(frame, data):
                ...
                ...

            pipeline.modifiers.append(my_mod_function)

       Your modifier function -which can have an arbitrary name such as ``my_mod_function()`` in this case- is inserted into the pipeline
       by appending it to the :py:attr:`Pipeline.modifiers <ovito.pipeline.Pipeline.modifiers>` list. Behind the scenes, OVITO automatically creates a
       :py:class:`~ovito.modifiers.PythonScriptModifier` instance to wrap the Python function object.

Keep in mind that OVITO is going to invoke your Python function whenever it needs to, and as many times as it needs to. Typically this will happen
when the pipeline is being evaluated. In the graphical program, a pipeline evaluation routinely occurs as part of updating the interactive
viewports or when you render an image. In a batch script you typically request the pipeline evaluation explicitly
by calling :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` or indirectly by invoking a function such as :py:func:`~ovito.io.export_file`.

---------------------------------------
Implementing the modifier function
---------------------------------------

When OVITO's pipeline system calls your modifier function, it passes it the :py:class:`~ovito.data.DataCollection` produced by the upstream part of the pipeline
(e.g. data loaded by the input :py:class:`~ovito.pipeline.FileSource` and further processed by any modifiers
preceding the user-defined modifier in the pipeline). Your Python modifier function then has the option to modify or amend
the :py:class:`~ovito.data.DataCollection` in some way. After your modifier function has done its work and returns,
the modified data state is automatically passed on to subsequent modifiers and continues flowing down the pipeline.

It is important to note that a user-defined modifier function is subject to certain restrictions.
Since it will get called by the pipeline system as needed in a callback fashion, the function may manipulate
only the :py:class:`~ovito.data.DataCollection` object it receives through the ``data`` function parameter and nothing else.
In particular it must not manipulate the pipeline structure itself (e.g. add/remove modifiers) or perform other operations that
have side effects on the global program state.

Initialization phase
-----------------------------------

.. warning::
   The following sections on this page are out of date! They have not been updated yet to reflect the changes made in the current
   development version of OVITO.

Initialization of parameters and other inputs needed by our custom modifier function should be done outside of the function.
For example, our modifier may require reference coordinates of particles, which need to be loaded from an external file.
One example is the *Displacement vectors* modifier of OVITO, which asks the user to load a reference configuration file with the
coordinates that should be subtracted from the current particle coordinates. A corresponding implementation of this modifier in Python
would look as follows::

    from ovito.io import FileSource

    reference = FileSource()
    reference.load("simulation.0.dump")

    def modify(frame, data):
        prop = output.create_particle_property(ParticleProperty.Type.Displacement)

        prop.marray[:] = (    input.particle_properties.position.array -
                          reference.particle_properties.position.array)

The script above creates a :py:class:`~ovito.io.FileSource` to load the reference particle positions from an external
data file. Within the actual ``modify()`` function we can then access the particle
coordinates loaded by the :py:class:`~ovito.io.FileSource` object.

Long-running modifier functions
------------------------------------------------------

Due to technical limitations the custom modifier function is always executed in the main thread of the application.
This is in contrast to the built-in asynchronous modifiers of OVITO, which are implemented in C++.
They are executed in a background thread to not block the graphical user interface during long-running operations.

That means, if our Python modifier function takes a long time to compute before returning control to OVITO, no input events
can be processed by the application and the user interface will freeze. To avoid this, you can make your modifier function asynchronous using
the ``yield`` Python statement (see the `Python docs <https://docs.python.org/3/reference/expressions.html#yieldexpr>`__ for more information).
Calling ``yield`` within the modifier function temporarily yields control to the
main program, giving it the chance to process waiting user input events or repaint the viewports::

   def modify(frame, input, output):
       for i in range(input.number_of_particles):
           # Perform a small computation step
           ...
           # Temporarily yield control to the system
           yield

In general, ``yield`` should be called periodically and as frequently as possible, for example after processing one particle from the input as
in the code above.

The ``yield`` keyword also gives the user (and the system) the possibility to cancel the execution of the custom
modifier function. When the evaluation of the modification pipeline is interrupted by the system, the ``yield`` statement does not return
and the Python function execution is discontinued.

Finally, the ``yield`` mechanism gives the custom modifier function the possibility to report its progress back to the system.
The progress must be reported as a fraction in the range 0.0 to 1.0 using the ``yield`` statement. For example::

   def modify(frame, input, output):
       total_count = input.number_of_particles
       for i in range(0, total_count):
           ...
           yield (i/total_count)

The current progress value will be displayed in the status bar by OVITO.
Moreover, a string describing the current status can be yielded, which will also be displayed in the status bar::

   def modify(frame, input, output):
       yield "Performing an expensive analysis..."
       ...

-------------------------------------------------
Next topic
-------------------------------------------------

  * :ref:`rendering_intro`
