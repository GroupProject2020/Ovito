.. _writing_custom_modifiers:

===================================
User-defined modifiers
===================================

OVITO's Python programming interface allows you to write your own modifier functions that participate in the
:ovitoman:`data pipeline system <../../usage.modification_pipeline>` of OVITO. Writing your own modifier function
is useful in situations where the built-in modifiers of OVITO (found in the :py:mod:`ovito.modifiers` module) are not sufficient
to solve your specific problem at hand.

----------------------------------------------
Defining a modifier function
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

            # Insert the modifier function into a pipeline:
            pipeline.modifiers.append(my_mod_function)

            # Pipeline evaluation: The system will invoke your user-defined function.
            data = pipeline.compute()

       Your modifier function -which can have an arbitrary name such as ``my_mod_function()`` in this case- is inserted into the pipeline
       by appending it to the :py:attr:`Pipeline.modifiers <ovito.pipeline.Pipeline.modifiers>` list. Behind the scenes, OVITO automatically wraps
       the Python function object in a :py:class:`~ovito.modifiers.PythonScriptModifier` instance.

Keep in mind that OVITO is going to invoke your Python function whenever it needs to, and as many times as it needs to. Typically this will happen
when the pipeline is being evaluated. In the graphical program, a pipeline evaluation routinely occurs as part of updating the interactive
viewports or when you render an image. In a batch script you typically request the pipeline evaluation explicitly
by calling :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` or indirectly by invoking a function such as :py:func:`~ovito.io.export_file`.

---------------------------------------
Implementing a modifier function
---------------------------------------

When OVITO's pipeline system calls your modifier function, it passes it the :py:class:`~ovito.data.DataCollection` produced by the upstream part of the pipeline
(e.g. typically some data loaded by the pipeline's :py:class:`~ovito.pipeline.FileSource` and further processed by any modifiers
preceding the user-defined modifier in the pipeline). Your Python modifier function then has the option to modify or amend
the :py:class:`~ovito.data.DataCollection` in some way. After your modifier function has done its work and returns,
the modified data state is automatically passed on to any subsequent modifiers down the pipeline.

Writing well-behaved modifier functions
----------------------------------------

It is important to note that a user-defined modifier function is subject to certain restrictions.
Since it will get called by the pipeline system as needed, the function should only manipulate
the :py:class:`~ovito.data.DataCollection` it receives through the ``data`` function parameter and nothing else.
In particular, it must not modify the pipeline structure itself (e.g. add/remove modifiers) or perform other operations that
have side effects on the global program state. Here are a few examples for things you should typically *not* do inside a user-defined modifier function,
because they can lead to undesired side effects::

    total_energy = 0.0  # A global variable accessed below

    def modify(time, data):

        # Do NOT add modifiers to the current pipeline (because any changes made to a pipeline
        # while it is being evaluated will lead to an infinite loop):
        ovito.scene.selected_pipeline.modifiers.append(...)

        # Do NOT insert new objects into the current scene (because a modifier function is only
        # supposed to modify the data collection flowing down the pipeline):
        pipeline2 = import_file(...)
        pipeline2.add_to_scene()

        # Do NOT modify global variables or objects (because the system may call your
        # function an arbitrary number of times):
        total_energy += numpy.sum(data.particles['Potential Energy'])

        # Do NOT perform file I/O from within a modifier function, because you don't know when
        # and how often the pipeline system is going to call your function:
        file = open('my_output.txt', 'w')

When implementing a modifier function that alters the contents of the :py:class:`~ovito.data.DataCollection` passed in by
the system, make sure you adhere to the rules of :ref:`shared data ownership <data_ownership>` and make use of the :ref:`underscore notation <underscore_notation>`
to announce any modifications your are going make to data objects. See the :ref:`examples section <modifier_script_examples>` of this manual, which provides various examples
of user-defined modifier functions.

How to save computed information to disk
-------------------------------------------------

A typical use case for custom analysis functions is the computation of some specific information for each frame of a simulation
trajectory. The analysis results often need to be written to disk in order to subsequently use them (e.g. for creating a data plot).
As mentioned in the previous section, however, the user-defined modifier function itself should not directly write data to disk. Instead, the
information should be fed back to the data pipeline of OVITO by storing it in the :py:class:`~ovito.data.DataCollection` object.
You can subsequently use OVITO's standard file export function to write the results of the data pipeline to disk (accessible from the file menu in
the GUI, or by calling the :py:func:`~ovito.io.export_file` Python function in a batch script).

Simple quantities computed by your modifier function can be :ref:`output as global attributes <adding_global_attributes>`
by storing them in the :py:attr:`DataCollection.attributes <ovito.data.DataCollection.attributes>` dictionary. You can then
export the attribute(s) to a text file for all simulation frames by invoking the :py:func:`~ovito.io.export_file` function with the
``"txt/attr"`` output format (or by selecting the "Table of values" format in the GUI). See the :ref:`this example <example_msd_calculation>`.

A similar approach should be followed when your modifier functions computes some information for each particle that needs to
be written to disk. In this case, the function should stores the computed per-particle information as a :ref:`new particle property <creating_new_properties>`
and then use OVITO's file export function to write the pipeline results out to disk (e.g. in the simple XYZ file format).

In more complicated situations, for example when the computed information needs to be written to a file in a custom format, a different approach
may be more suitable. Instead of performing the computation within a user-defined modifier function that gets called by the pipeline system, you
should rather do the analysis in a :ref:`batch script <scripting_running>` looping over all simulation frames::

    pipeline = import_file(...)
    for frame in range(pipeline.source.num_frames):
        data = pipeline.compute(frame)
        ...
        # Perform analysis of the current frame...
        ...
        # Write results to an output file using Python functions
        file = open('output_file.%i.txt' % frame, 'w')
        ...

Note that this approach requires that you execute the batch script outlined above through the :ref:`ovitos <ovitos_interpreter>`
script interpreter in the system terminal. It cannot be used within the graphical program environment.

One-time loading of input data
---------------------------------------

Some user-defined modifier functions may require additional input data that needs to be read from disk. For example, this could be
extra per-particle information stored in a separate file. Loading such auxiliary information should be done outside of the modifier function,
in particular if the information is static, i.e., does not depend on the current simulation frame.

Consider as an example the *Displacement vectors* modifier of OVITO, which lets the user load a separate file
containing the reference particle coordinates that should be subtracted from the current positions. A corresponding implementation of this modifier
in Python would look as follows:

.. literalinclude:: ../example_snippets/displacement_vector_calculation.py
  :lines: 4-14

Long-running modifier functions
------------------------------------------------------

The user-defined modifier function is always executed in the main thread of the graphical application. That means, if our Python function takes a
long time to execute before returning control to the system, no mouse or keyboard input events can be processed by the application and
the user interface will freeze. To avoid this situation, you can make your modifier function asynchronous by including
one or more ``yield`` Python statements (see the `Python docs <https://docs.python.org/3/reference/expressions.html#yieldexpr>`__ for more information).
Calling ``yield`` within the modifier function temporarily yields control back to the
main program, giving it the chance to process waiting user input events or repaint the viewports::

    def modify(frame, data):
        # This is a long-running loop over all particles in the system:
        for i in range(data.particles.count):
            # Perform one short computational step:
            ...
            # Temporarily yield control to the system to process input events:
            yield

``yield`` should be called periodically and as frequently as possible, for example after processing one input element at a time as
in the code above. The ``yield`` keyword also gives the system the possibility to cancel the execution of the
modifier function. When the evaluation of the data pipeline is interrupted by the system, the ``yield`` statement does not return
control back to the function and execution is discontinued.

Finally, the ``yield`` mechanism also gives the modifier function the possibility to report its progress back to the system.
The current progress must be specified as a fraction in the range 0.0-1.0 using the ``yield`` statement. For example::

   def modify(frame, data):
       for i in range(data.particles.count):
           ...
           yield (i / data.particles.count)

The reported progress value will be displayed in the status bar of OVITO while the modifier function is executing.
Moreover, a string describing the current operation can be passed to the system, which will also be displayed in the status bar::

   def modify(frame, data):
       yield "Performing an analysis..."
       ...

-------------------------------------------------
Next topic
-------------------------------------------------

  * :ref:`rendering_intro`
