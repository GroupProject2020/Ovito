==================================
Running scripts
==================================

OVITO's scripting interface enables you to automate visualization and analysis tasks and to extend the 
program (e.g. by adding your own data modification or analysis functions). 
The following sections provide a short overview of the different ways scripting can used in OVITO --
and how OVITO's capabilities can be leveraged from scripts:

**Invoking program functions** 

Scripts can invoke program functions, similar to what a human user does in the graphical interface.
For example, you can write a Python script file and execute it within the context of the current program
session using the ``Run Script File`` function from the ``Scripting`` menu of OVITO. 
Such a *macro* script can insert new modifiers into the current data pipeline, access the results of the 
pipeline and export the data to an output file. Typically, such a macro script operates on data that is already 
loaded in the current OVITO program session.
 
**Batch-processing** 

More commonly, however, you want to fully automate processing or visualization tasks from beginning to end: (1) loading input data, (2) processing or analyzing it, 
(3) saving results to disk. Such a *batch* script is executed from the terminal using the external :program:`ovitos` script interpreter, 
which works like a normal Python interpreter and which will be introduced below. This approach can
be used, for instance, to process a large number of simulation snapshots right on the computing cluster where the data is stored.
 
**Extending OVITO: User-defined modifiers** 

OVITO's scripting framework also gives you the possibility to develop new kinds of modifiers that manipulate 
or analyze simulation data in ways not covered by any of the built-in modifiers of the program. 
So-called *Python script modifiers* (see :ref:`writing_custom_modifiers` section) participate in the data pipeline system of 
OVITO and behave like the built-in modifiers from a user's perspective. A *Python script modifier* essentially is
a callable Python function, which you write. It is executed automatically by the system whenever the data pipeline it has been inserted into gets evaluated.

**Extending OVITO: User-defined viewport overlays** 

Similar to scripted modifiers, you can also write custom viewport overlays. 
A `Python script overlay <../../viewport_overlays.python_script.html>`_ is a user-defined function that gets called by OVITO every time 
an interactive viewport is repainted or an image is rendered. This allows you to enrich images or movies rendered by OVITO with custom graphics or text, e.g., to include 
additional information like scale bars or data plots with information dynamically computed by OVITO.

Note that *Python script modifiers* and *Python viewport overlays* can be used both from within the graphical user interface 
and from non-interactive batch scripts. In the first case, the user inserts the Python modifier in the
graphical interface of OVITO and enters the function code in the integrated code editor. In the second case,
the custom modifier or overlay function is embedded in the batch script (see :py:class:`~ovito.modifiers.PythonScriptModifier` class for an example).

OVITO's Python interpreter
----------------------------------

OVITO comes with a script interpreter, which can execute programs written in the Python language.
The current version of OVITO is compatible with the `Python 3.5 <https://docs.python.org/3.5/>`_ language standard. 
You typically execute batch Python scripts from the terminal of your operating system using the :program:`ovitos` script interpreter that is installed 
along with OVITO:

.. code-block:: shell-session

	ovitos [-o file] [-g] [script.py] [args...]
	
The :program:`ovitos` program is located in the :file:`bin/` subdirectory of OVITO for Linux, in the 
:file:`Ovito.app/Contents/MacOS/` directory of OVITO for macOS, and in the main application directory 
on Windows systems. It should not be confused with :program:`ovito`, the graphical program with the 
interative user interface.

Let's use a text editor to write a simple Python script file named :file:`hello.py`::

	import ovito
	print("Hello, this is OVITO %i.%i.%i" % ovito.version)

We can then execute the script from a Linux terminal as follows:

.. code-block:: shell-session

	me@linux:~/ovito-3.0.0-x86_64/bin$ ./ovitos hello.py
	Hello, this is OVITO 3.0.0
	
By default, the :program:`ovitos` script interpreter displays only console output and no graphical output.
This allows running OVITO scripts on remote machines or computing clusters that don't possess a graphics display. 
The :program:`ovitos` program behaves like a standard Python interpreter. Any command line arguments following the 
script's name are passed on to the script via the ``sys.argv`` variable. Furthermore, it is possible to start 
an interactive interpreter session by running :program:`ovitos` without any arguments.

Preloading program state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :command:`-o` command line option lets :program:`ovitos` load an :file:`.ovito` state file before executing the
script. This allows you to preload an existing visualization setup that you have 
previously prepared with the graphical version of OVITO and saved to a :file:`.ovito` file. This can save you programming
work, because modifiers, parameters and the camera setup get already loaded from the state file and 
don't need to be set up programmatically in the batch script anymore.

Activate graphical mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :command:`-g` command line option of the script interpreter starts a graphical program session and the script
will be run in the context of OVITO's main window. This allows you to follow your script commands as they are being 
executed. This is useful, for instance, if you want to visually check the outcome of your script's action during the 
development phase. Keep in mind that the viewports will only show pipelines that are part of the current scene. 
Thus, it may be necessary to explicitly call :py:meth:`Pipeline.add_to_scene() <ovito.pipeline.Pipeline.add_to_scene>`
to make your imported data visible in this mode.

Number of parallel threads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

OVITO uses all available processor cores by default to perform computations. To restrict OVITO
to a certain number of parallel threads, use the :command:`--nthreads` command line parameter, e.g. :command:`ovitos --nthreads 1 myscript.py`.

Third-party Python modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The embedded script interpreter of OVITO is a preconfigured version of the standard `CPython <https://en.wikipedia.org/wiki/CPython>`_ interpreter with the
:py:mod:`ovito` Python package included. This makes it possible to run scripts both within the graphical program OVITO as well as through the :program:`ovitos`
command line interpreter. However, the :program:`ovitos` interpreter includes only the `NumPy <http://www.numpy.org/>`_, `matplotlib <http://matplotlib.org/>`_, 
and `PyQt5 <http://pyqt.sourceforge.net/Docs/PyQt5/>`_ packages as preinstalled extensions.

If you want to call other third-party Python modules from your OVITO scripts, it may be possible to install them in the 
:program:`ovitos` interpreter using the normal *pip* or *setuptools* mechanisms 
(e.g., run :command:`ovitos -m pip install <package>` to install a module via *pip*).

Installing Python extensions that include native code (e.g. `Scipy <http://www.scipy.org>`_) in the embedded interpreter 
will likely fail, however. It is recommended to build OVITO from source on your local system in this case. 
The graphical program as well as :program:`ovitos` will then make use of your system's standard Python installation.
This makes all modules that are installed in your system interpreter accessible within OVITO and :program:`ovitos` as well.
How to build OVITO from source is described `on this page <http://www.ovito.org/manual/development.html>`_.

Using the ovito package from other Python interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :py:mod:`ovito` Python package can also be imported by Python scripts running in an external Python interpreter, not :program:`ovitos`. 
However, because this module contains native extensions, it must be compiled specifically for the Python interpreter being used. 
Since there is a chance that the binary extension module shipped with the prebuilt version of OVITO is not compatible 
with your local Python interpreter, it might be necessary to `build OVITO from source <http://www.ovito.org/manual/development.html>`_.
In case you have multiple Python interpreters installed on your system, make sure OVITO is being linked against the 
version that you are going to run your scripts with.

Once the graphical program and the :py:mod:`ovito` Python module have been successfully built, 
you should add the following directory to the `PYTHONPATH <https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH>`_,
so that the Python interpreter can find it:

=============== ===========================================================
Platform:        Location of ovito package relative to build path:
=============== ===========================================================
Windows         :file:`plugins/python/`
Linux           :file:`lib/ovito/plugins/python/`
macOS           :file:`Ovito.app/Contents/Resources/python/`
=============== ===========================================================
