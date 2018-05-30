# Load the native modules and other dependencies.
from ..plugins.PyScript import FileImporter, FileSource, ImportMode, PipelineStatus
from ..data import DataCollection
from ..pipeline import Pipeline
import ovito
import sys
try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

def import_file(location, **params):
    """ Imports data from an external file. 
    
        This Python function corresponds to the *Load File* menu command in OVITO's
        user interface. The format of the imported file is automatically detected (see `list of supported formats <../../usage.import.html#usage.import.formats>`__).
        Depending on the file's format, additional keyword parameters may be required 
        to specify how the data should be interpreted. These keyword parameters are documented below.
        
        :param location: The path to import. This can be a local file or a remote *sftp://* URL.
        :returns: The new :py:class:`~ovito.pipeline.Pipeline` that has been created for the imported data.

        The function creates and returns a new :py:class:`~ovito.pipeline.Pipeline` object, which uses the contents of the
        external data file as input. The pipeline will be wired to a :py:class:`~ovito.pipeline.FileSource`, which 
        reads the input data from the external file and passes it on to the pipeline. You can access the 
        data by calling the :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` method or, alternatively,
        :py:meth:`FileSource.compute() <ovito.pipeline.FileSource.compute>` on the data :py:attr:`~ovito.pipeline.Pipeline.source`.
        As long as the new :py:class:`~ovito.pipeline.Pipeline` contains no modifiers yet, both methods will return the same data.
        
        Note that the :py:class:`~ovito.pipeline.Pipeline` is not automatically inserted into the three-dimensional scene. 
        That means the loaded data won't appear in rendered images or the interactive viewports of OVITO by default.
        For that to happen, you need to explicitly insert the pipeline into the scene by calling its :py:meth:`~ovito.pipeline.Pipeline.add_to_scene` method if desired.
        
        Furthermore, note that you can re-use the returned :py:class:`~ovito.pipeline.Pipeline` if you want to load a different 
        data file later on. Instead of calling :py:func:`!import_file` again to load another file,
        you can use the :py:meth:`pipeline.source.load(...) <ovito.pipeline.FileSource.load>` method to replace the input file 
        of the already existing pipeline.
        
        **File columns**
        
        When importing XYZ files or *binary* LAMMPS dump files, the mapping of file columns 
        to OVITO's particle properties must be specified using the ``columns`` keyword parameter::
        
            pipeline = import_file("file.xyz", columns = 
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
        
        The length of the string list must match the number of data columns in the input file. 
        See this :ref:`table <particle-types-list>` for standard particle property names. Alternatively, you can specify
        user-defined names for file colums that should be read as custom particle properties by OVITO.
        For vector properties, the component name must be appended to the property's base name as demonstrated for the ``Position`` property in the example above. 
        To completely ignore a file column during import, specify ``None`` in the ``columns`` list.
        
        For *text-based* LAMMPS dump files, OVITO automatically determines a reasonable column-to-property mapping, but you may override it using the
        ``columns`` keyword. This can make sense, for example, if the file columns containing the particle coordinates
        do not follow the standard naming scheme ``x``, ``y``, and ``z`` (e.g. when reading time-averaged atomic positions computed by LAMMPS). 
        
        **Frame sequences**
        
        OVITO automatically detects if the imported file contains multiple data frames (timesteps). 
        Alternatively (and additionally), it is possible to load a sequence of files in the same directory by using the ``*`` wildcard character 
        in the filename. Note that ``*`` may appear only once, only in the filename component of the path, and only in place of numeric digits. 
        Furthermore, it is possible to pass an explicit list of file paths to the :py:func:`!import_file` function, which will be loaded
        as an animatable sequence. All variants can be combined. For example, to load two file sets from different directories as one
        consecutive sequence::

           import_file('sim.xyz')     # Loads all frames from the given file 
           import_file('sim.*.xyz')   # Loads 'sim.0.xyz', 'sim.100.xyz', 'sim.200.xyz', etc.
           import_file(['sim_a.xyz', 'sim_b.xyz'])  # Loads an explicit list of files
           import_file([
                'dir_a/sim.*.xyz', 
                'dir_b/sim.*.xyz']) # Loads several file sequences from different directories
        
        The number of frames found in the input file(s) is reported by the :py:attr:`~ovito.pipeline.FileSource.num_frames` attribute of the pipeline's :py:class:`~ovito.pipeline.FileSource`
        You can step through the frames with a ``for``-loop as follows:
        
        .. literalinclude:: ../example_snippets/import_access_animation_frames.py
         
        **LAMMPS atom style**
        
        When loading a LAMMPS data file that is based on an atom style other than "atomic", the style may need to be
        specified using the ``atom_style`` keyword parameter. Only the following LAMMPS atom styles are currently supported by
        OVITO: ``angle``, ``atomic``, ``body``, ``bond``, ``charge``, ``dipole``, ``full``, ``molecular``.
        LAMMPS data files generated by LAMMPS's own ``write_data`` command contain a hint that indicates the atom style.
        Such files can be loaded without specifying the ``atom_style`` keyword.

        **Topology and trajectory files**

        Some simulation codes write a *topology* file and separate *trajectory* file. The former contains only static information like the bonding
        between atoms, the atom types, etc., which do not change during a simulation run, while the latter stores the varying data (primarily
        the atomic trajectories). To load such a topology-trajectory pair of files, first read the topology file with
        the :py:func:`!import_file` function, then insert a :py:class:`~ovito.modifiers.LoadTrajectoryModifier` into the returned :py:class:`~ovito.pipeline.Pipeline`
        to also load the trajectory data.
    """

    # Process input parameter
    if isinstance(location, str if sys.version_info[0] >= 3 else basestring):
        location_list = [location]
    elif isinstance(location, collections.Sequence):
        location_list = list(location)
    else:
        raise TypeError("Invalid input file location. Expected string or sequence of strings.")
    first_location = location_list[0]

    # Importing a file is a long-running operation, which is not permitted during viewport rendering or pipeline evaluation.
    # In these situations, the following function call will raise an exception.
    ovito.dataset.request_long_operation()
    
    # Determine the file's format.
    importer = FileImporter.autodetect_format(ovito.dataset, first_location)
    if not importer:
        raise RuntimeError("Could not detect the file format. The format might not be supported.")
    
    # Forward user parameters to the file importer object.
    for key in params:
        if not hasattr(importer, key):
            raise RuntimeError("Invalid keyword parameter. Object %s doesn't have a parameter named '%s'." % (repr(importer), key))
        importer.__setattr__(key, params[key])

    # Import data.
    if not importer.import_file(location_list, ImportMode.AddToScene, False):
        raise RuntimeError("Operation has been canceled by the user.")

    # Get the newly created pipeline.
    pipeline = ovito.scene.selected_pipeline
    if not isinstance(pipeline, Pipeline):
        raise RuntimeError("File import failed. Nothing was imported.")
    
    try:
        # Block execution until file is loaded.
        state = pipeline.evaluate_pipeline(0)  # Requesting frame 0 here, because full list of frames is not loaded yet at this point.
        
        # Raise exception if error occurs during loading.
        if state.status.type == PipelineStatus.Type.Error:
            raise RuntimeError(state.status.text)

        # Block until full list of animation frames has been obtained.
        if isinstance(pipeline.source, FileSource) and not pipeline.source.wait_for_frames_list():
            raise RuntimeError("Operation has been canceled by the user.")
    except:
        # Delete newly created pipeline in case import fails.
        pipeline.delete()
        raise
    
    # The importer will insert the pipeline into the scene in the GUI by default.
    # But in the scripting enviroment we immediately undo it.
    pipeline.remove_from_scene()
    
    return pipeline    