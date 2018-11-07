.. _scripting_running:

==================================
Using scripting features
==================================

OVITO's scripting interface enables you to automate visualization and analysis tasks and to implement your own data manipulation and analysis 
algorithms. The scripting interface consists of a set of Python classes and functions that provide access to the program's
internal data model, data processing framework and visualization functions. For a new user, it can be somewhat confusing at first that 
the scripting capabilities can be used in multiple ways: On one hand, the graphical OVITO application contains an embedded Python interpreter,
which lets you execute Python scripts in the context of the running application. Such scripts can operate on the currently 
loaded dataset and allow you to extend the capabilities of the interactive OVITO application. 
On the other hand, it is possible to execute automation scripts from the system terminal without opening the 
interactive OVITO application. This allows you to leverage the analysis and visualization features of the program package
and integrate them into fully automated post-processing and visualization workflows. 
The following paragraphs provide a brief overview of the different ways to use OVITO's scripting interface:

**Batch scripts** 

Batch scripts are a way to integrate OVITO's powerful data processing and visualization capabilities into custom
workflows for post-processing of simulations. Such script are typically run from the system terminal using the :program:`ovitos` 
script interpreter (not :program:`ovito`!), which works similar to a standard Python interpreter and which will be introduced below. 
Batch scripts run non-interactively and outside the graphical user interface, but they can leverage most program features that
you already know from the interactive OVITO application, e.g. loading simulation data, setting up data pipelines, rendering pictures and animations,
accessing computation results and exporting data to output files.
 
**Macro scripts** 

The *Run Script File* function from the *File* menu of the graphical OVITO program lets you execute a Python script file at any time
within the graphical user interface. The script will be executed in the context of the active program session and 
may invoke program functions just like a human user of the graphical interface can. 
For example, your script may insert certain modifiers into the current data pipeline or export the data of the current pipeline to an 
output file. 

This type of script is typically used to perform sequences of program actions that you need frequently. 
Instead of carrying them out by hand, you let the script invoke the actions for you, similar to an *application macro*. 
Note that OVITO furthermore offers the :command:`--script` command line option, which runs a macro script right after application startup.

**User-defined modifiers and viewport overlays** 

OVITO's scripting interface also enables you to develop new kinds of modifiers that manipulate or analyze data 
in ways not covered by any of the built-in modifiers of the program. So-called *Python script modifiers* (see :ref:`writing_custom_modifiers` section) 
participate in the data pipeline system of OVITO and behave just like the built-in modifiers from a user's perspective. 
A *Python script modifier* essentially is a callable Python function that you write. It is executed automatically and repeatedly by the 
system whenever the data pipeline gets evaluated. 

Similar to modifiers, you can write custom viewport overlays. A `Python script viewport overlay <../../viewport_overlays.python_script.html>`__ is a 
user-defined function that gets called by OVITO every time a viewport image is being rendered. This allows you to enrich images or movies rendered by 
OVITO with custom graphics or text and include additional information like scale bars or data plots dynamically generated from information in OVITO.

Note that *Python script modifiers* and *Python viewport overlays* can be used from within the graphical user interface 
as well as from non-interactive batch scripts. In the first case, the user actively inserts a Python modifier into the
data pipeline and enters the code for the user-defined modifier function into the integrated code editor. In the latter case,
the code of the custom modifier or overlay function is part of the batch script itself (see :py:class:`~ovito.modifiers.PythonScriptModifier` class for an example).


OVITO's Python interpreter
----------------------------------

OVITO comes with a script interpreter, which can execute programs written in the Python language.
The current version of OVITO is compatible with the `Python 3.6 <https://docs.python.org/3.6/>`__ language standard. 
You typically execute batch Python scripts from the terminal of your operating system using the :program:`ovitos` script interpreter that gets installed 
along with OVITO:

.. code-block:: shell-session

	ovitos [-o file] [-g] [script.py] [args...]
	
The :program:`ovitos` program is located in the :file:`bin/` subdirectory of OVITO for Linux, in the 
:file:`Ovito.app/Contents/MacOS/` directory of OVITO for macOS, and in the main application directory 
on Windows systems. It should not be confused with :program:`ovito`, the graphical program with an 
interactive user interface.

Let's use a text editor to write a simple Python script file named :file:`hello.py`::

	import ovito
	print("Hello, this is OVITO %i.%i.%i" % ovito.version)

We can execute the script file from a Linux terminal as follows:

.. code-block:: shell-session

	me@linux:~/ovito-3.0.0-x86_64/bin$ ./ovitos hello.py
	Hello, this is OVITO 3.0.0
	
The :program:`ovitos` script interpreter is a console program without a graphical user interface.
This allows running OVITO scripts on remote machines or computing clusters that don't possess a graphics display. 
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

Number of parallel threads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

OVITO uses all available processor cores by default to perform computations. To restrict the program
to a certain maximum number of parallel threads, use the :command:`--nthreads` command line parameter, e.g. :command:`ovitos --nthreads 1 myscript.py`.

Third-party Python modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The embedded script interpreter of OVITO is a preconfigured version of the standard `CPython <https://en.wikipedia.org/wiki/CPython>`__ interpreter with the
:py:mod:`ovito` Python package included. This makes it possible to run scripts both within the graphical program OVITO as well as through the :program:`ovitos`
command line interpreter. However, OVITO's Python interpreter only includes the `NumPy <http://www.numpy.org/>`__, `matplotlib <http://matplotlib.org/>`__, 
and `PyQt5 <http://pyqt.sourceforge.net/Docs/PyQt5/>`__ packages as preinstalled extensions.

If you want to call other third-party Python modules from your OVITO scripts, it may be possible to install them in the 
:program:`ovitos` interpreter using the normal *pip* or *setuptools* mechanisms 
(e.g., run :command:`ovitos -m pip install <package>` to install a module via *pip*).

Installing Python extensions that include native code may fail, however, because such extensions may not be compatible 
with the build-time configuration of the embedded interpreter. In this case, it is recommended to build OVITO from source on your local 
system. The graphical program as well as :program:`ovitos` will then make use of your system's standard Python installation.
This makes all modules that are installed in your system interpreter also accessible within OVITO and :program:`ovitos`.
Instructions how to build OVITO from source can be found in the `user manual <http://www.ovito.org/manual/development.html>`__.

Using the ovito package from other Python interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :py:mod:`ovito` Python package can also be imported by Python scripts running in an external Python interpreter other than :program:`ovitos`. 
However, because the :py:mod:`ovito` module contains native extensions, it must be compiled specifically for the Python interpreter being used with. 
Since there is a chance that the binary extension module shipping with the prebuilt version of OVITO is not compatible 
with your local Python interpreter, it may be necessary to `build OVITO from source <http://www.ovito.org/manual/development.html>`__.
In case you have multiple Python versions installed on your system, pay attention that OVITO is being built against the 
version that you will use for running scripts.

Once the graphical program and the :py:mod:`ovito` Python module have been successfully built, 
you should add the following directories from the build path to the `PYTHONPATH <https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH>`__ 
environment variable, so that your Python interpreter can find the module:

=============== ===========================================================
Platform:        Location of ovito package relative to build path:
=============== ===========================================================
Windows         :file:`plugins/python/`
Linux           :file:`lib/ovito/plugins/python/`
macOS           :file:`Ovito.app/Contents/Resources/python/`
=============== ===========================================================
