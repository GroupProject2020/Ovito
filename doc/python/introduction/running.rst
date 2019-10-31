.. _scripting_running:

==================================
How to run scripts
==================================

OVITO's scripting interface enables you to automate visualization and analysis tasks and to implement your own data manipulation and analysis
algorithms. The scripting interface consists of a set of Python classes and functions that provide access to the program's
internal data model, data processing framework and visualization functions. For a new user, it can be somewhat confusing at first that
the scripting capabilities can be used in multiple ways: On one hand, the graphical OVITO application contains an embedded Python interpreter,
which lets you execute Python scripts in the context of the current interactive program session. Such scripts can operate on the currently
loaded dataset and allow you to extend the capabilities of the graphical OVITO application.
On the other hand, it is possible to execute automation scripts (batch scripts) from the system terminal without ever opening the
OVITO application. This allows you to leverage the analysis and visualization capabilities of OVITO
and build fully automated post-processing and visualization workflows.
The following paragraphs provide brief overviews of these different ways of using OVITO's scripting interface:

**Batch scripts**

Batch scripts are a way to integrate OVITO's powerful data processing and visualization functions into custom
workflows for post-processing of simulation data. Such script are typically executed from the system terminal using the :program:`ovitos`
script interpreter (not :program:`ovito`!), which behaves similar to a standard Python interpreter and which will be introduced below.
Batch scripts run non-interactively and without the graphical user interface, but they can leverage most program features that
you already know from the interactive OVITO application, e.g. loading simulation data, setting up data pipelines, rendering pictures and animations,
accessing computation results and exporting result data to an output file.

**Macro scripts**

The *Run Script File* function found in the *File* menu of OVITO lets you execute a Python script file within the running graphical user interface.
The code statements will be executed in the context of the current program session and may invoke program functions just like a human user can.
For example, your macro script may insert certain modifiers into the current data pipeline or export the data of the pipeline to an
output file in a specific way.

This type of script is typically used to perform sequences of program actions that you need frequently.
Instead of carrying them out by hand, you let the script invoke the actions for you, similar to an *application macro*.
Note that OVITO offers the :command:`--script` command line option, which lets you run a macro script right after application startup,
for example to initialize the program session to a specific default state.

**User-defined modifiers and viewport overlays**

OVITO's scripting interface also enables you to develop new kinds of modifiers that manipulate or analyze data
in ways not covered by any of the built-in modifiers of the program. So-called *Python script modifiers* (see :ref:`writing_custom_modifiers` section)
participate in the data pipeline system of OVITO and behave just like the built-in modifiers from a user's perspective.
A *Python script modifier* essentially is a callable Python function that you write. It is executed automatically and repeatedly by the
system whenever the data pipeline gets evaluated.

Similar to modifiers, you can write custom viewport overlays. A :ovitoman:`Python script viewport overlay <../../viewport_overlays.python_script>` is a
user-defined function that gets called by OVITO every time a viewport image is being rendered. This allows you to enrich images or movies rendered by
OVITO with custom graphics or text and include additional information like scale bars or data plots dynamically generated from information in OVITO.

Note that *Python script modifiers* and *Python viewport overlays* can be used from within the graphical user interface
as well as from non-interactive batch scripts. In the first case, the user actively inserts a Python modifier into the
data pipeline and enters the code for the user-defined modifier function into the integrated code editor. In the latter case,
the code of the custom modifier or overlay function is part of the batch script itself (see :py:class:`~ovito.modifiers.PythonScriptModifier` class for an example).

.. _ovitos_interpreter:

OVITO's Python interpreter
----------------------------------

OVITO comes with a script interpreter named :program:`ovitos` that can execute programs written in the standard Python language.
The current version of this interpreter is compatible with the `Python 3.6 <https://docs.python.org/3.6/>`__ language standard.
You typically execute batch Python scripts from the terminal of your operating system using the :program:`ovitos` script interpreter, which gets installed
alongside with OVITO:

.. code-block:: shell-session

	ovitos [-o file] [-g] [script.py] [args...]

The :program:`ovitos` program is located in the :file:`bin/` subdirectory of OVITO for Linux, in the
:file:`Ovito.app/Contents/MacOS/` directory of OVITO for macOS, and in the main application directory
on Windows systems (look for :program:`ovitos.exe`). It should not be confused with :program:`ovito`, the main program
providing the graphical user interface.

Let's use a text editor to write a simple Python script file named :file:`hello.py`::

	import ovito
	print("Hello, this is OVITO %i.%i.%i" % ovito.version)

We can execute the script file from a Linux terminal as follows:

.. code-block:: shell-session

	me@linux:~/ovito-3.0.0-x86_64/bin$ ./ovitos hello.py
	Hello, this is OVITO 3.0.0

The :program:`ovitos` script interpreter is a console program without a graphical user interface.
This enables you to run scripts on remote machines or computing clusters that don't possess a graphics display.
:program:`ovitos` behaves like a regular Python interpreter. Any command line arguments following the
script's name are passed to the script via the ``sys.argv`` variable. Furthermore, it is possible to start
an interactive interpreter session by running :program:`ovitos` without any arguments.

.. _preloading_program_state:

Preloading program state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :command:`-o` command line option tells :program:`ovitos` to load an :file:`.ovito` state file before executing the
script. This allows you to preload an existing data pipeline or visualization setup that you have
previously prepared using the graphical version of OVITO. All actions of the script will subsequently be carried out in the context of this preloaded program state.
This can save you programming work, because things like modifiers and the camera setup already get loaded from the state file and
you don't need to set them up programmatically in the batch script anymore.

Graphical mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :command:`-g` command line option of the script interpreter starts a graphical program session and the script
will be run in the context of OVITO's main window. This allows you to follow your script commands as they are being
executed. This is useful for debugging purposes if you want to visually check the outcome of your script's action during the
development phase. Keep in mind that the viewports will only show pipelines that are part of the current scene.
Thus, it may be necessary to explicitly call :py:meth:`Pipeline.add_to_scene() <ovito.pipeline.Pipeline.add_to_scene>`
to make your imported data visible in this mode.

Number of CPU cores
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

OVITO uses all available processor cores by default to perform some computations. To explicitly restrict the program
to a certain maximum number of parallel threads, use the :command:`--nthreads` command line parameter, e.g. :command:`ovitos --nthreads 1 myscript.py`.

.. _use_ovito_with_system_interpreter:

Third-party Python modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The embedded script interpreter of OVITO is a preconfigured version of the standard `CPython <https://en.wikipedia.org/wiki/CPython>`__ interpreter with the
:py:mod:`ovito` Python package built in. This makes it possible to run scripts both within the graphical program OVITO as well as through the :program:`ovitos`
command line interpreter. However, OVITO's Python interpreter only ships with the `NumPy <http://www.numpy.org/>`__ and `matplotlib <http://matplotlib.org/>`__
preinstalled packages.

If you want to call other third-party Python packages from your OVITO scripts, it may be possible to install them in the
:program:`ovitos` interpreter using the normal *pip* or *setuptools* mechanisms
(e.g., run :command:`ovitos -m pip install <package>` to install a module from `PyPI <https://pypi.org>`__).

Installing Python extensions that include native code may fail, however, because such extensions may not be compatible
with the build-time configuration of the embedded CPython interpreter. In this case, it is recommend that you install
the :py:mod:`ovito` module in your system's Python interpreter. Further instructions can be found `here <https://pypi.org/project/ovito/>`__.
