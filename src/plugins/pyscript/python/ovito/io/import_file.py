# Load the native modules and other dependencies.
from ..plugins.PyScript.IO import FileImporter, FileSource, ImportMode
from ..plugins.PyScript.Scene import PipelineStatus
from ..data import DataCollection
from ..pipeline import Pipeline
import ovito

def import_file(location, **params):
    """ Imports data from an external file. 
    
        This Python function corresponds to the *Load File* command in OVITO's
        user interface. The format of the imported file is automatically detected.
        However, depending on the file's format, additional keyword parameters may need to be supplied 
        to specify how the data should be interpreted. These keyword parameters are documented below.
        
        :param str location: The data file to import. This can be a local file path or a remote *sftp://* URL.
        :returns: The :py:class:`~ovito.pipeline.Pipeline` that has been created for the imported data.

        The function creates and returns a new :py:class:`~ovito.pipeline.Pipeline` wired to a
        :py:class:`~ovito.pipeline.FileSource`, which is responsible for loading the selected input data from disk. The :py:func:`!import_file`
        function requests a first evaluation of the new pipeline in order to fill the pipeline's input cache
        with data from the file.
        
        Note that the newly created :py:class:`~ovito.pipeline.Pipeline` is not automatically inserted into the three-dimensional scene. 
        That means it won't appear in rendered images or the interactive viewports of the graphical OVITO program.
        You need to explicitly insert the pipeline into the scene by calling :py:meth:`~ovito.pipeline.Pipeline.add_to_scene` when desired.
        
        Furthermore, note that the returned :py:class:`~ovito.pipeline.Pipeline` may be re-used to load a different 
        input file later on. Instead of calling :py:func:`!import_file` again to load the next file,
        you can use the :py:meth:`pipeline.source.load(...) <ovito.pipeline.FileSource.load>` method to replace the input file 
        of the already existing pipeline.
        
        **File columns**
        
        When importing XYZ files or *binary* LAMMPS dump files, the mapping of file columns 
        to OVITO's particle properties must be specified using the ``columns`` keyword parameter::
        
            pipeline = import_file("file.xyz", columns = 
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
        
        The number of list entries must match the number of data columns in the input file. 
        See the list of :ref:`standard particle properties <particle-types-list>` for possible target properties. If you specify a 
        non-standard property name for a column, a user-defined :py:class:`~ovito.data.ParticleProperty` object is created from the values in that file column.
        For vector properties, the component name must be appended to the property's base name as demonstrated for the ``Position`` property in the example above. 
        To completely ignore a file column during import, specify ``None`` at the corresponding position in the ``columns`` list.
        
        For *text-based* LAMMPS dump files, OVITO automatically determines a reasonable column-to-property mapping, but you may override it using the
        ``columns`` keyword. This can make sense, for example, if the file columns containing the particle coordinates
        do not follow the standard name scheme ``x``, ``y``, and ``z`` (e.g. when reading time-averaged atomic positions computed by LAMMPS). 
        
        **File sequences**
        
        You can import a sequence of files by passing a filename containing a ``*`` wildcard character to :py:func:`!import_file`. There may be 
        only one ``*`` in the filename (and not in the directory names). The wildcard matches only to numbers in a filename.
        
        OVITO scans the directory and imports all matching files that belong to the sequence. Note that OVITO only loads the first file into memory though.
        
        The length of the imported time series is reported by the :py:attr:`~ovito.io.FileSource.num_frames` field of the pipeline's :py:class:`~ovito.io.FileSource`
        and is also reflected by the global :py:class:`~ovito.anim.AnimationSettings` object. You can step through the frames of the animation
        sequence as follows:
        
        .. literalinclude:: ../example_snippets/import_access_animation_frames.py
        
        **Multi-timestep files**
        
        Some file formats store multiple frames in one file. In some cases OVITO cannot know (e.g. for XYZ and LAMMPS dump formats)
        that a file contains multiple frames (because, by default, reading the entire file is avoided for performance reasons). 
        Then it is necessary to explicitly tell OVITO to scan the entire file and load a sequence of frames by supplying the ``multiple_frames`` 
        option:: 
        
            pipeline = import_file("file.dump", multiple_frames = True)
 
        **LAMMPS atom style**
        
        When loading a LAMMPS data file that is based on an atom style other than "atomic", the style must be explicitly
        specified using the ``atom_style`` keyword parameter. Only the following LAMMPS atom styles are currently supported by
        OVITO: ``angle``, ``atomic``, ``body``, ``bond``, ``charge``, ``dipole``, ``full``, ``molecular``.
        LAMMPS data files may contain a hint indicating the atom style (in particular files generated by LAMMPS's own ``write_data`` command contain that hint).
        Such files can be loaded without specifying the ``atom_style`` keyword.
            
        **Topology and trajectory files**

        Some simulation codes write a *topology* file and separate *trajectory* file. The former contains only static information like the bonding
        between atoms, the atom types, etc., which do not change during an MD simulation run, while the latter stores the varying data (first and foremost
        the atomic trajectories). To load such a topology-trajectory pair of files in OVITO, first load the topology file with
        the :py:func:`!import_file` function, then insert a :py:class:`~ovito.modifiers.LoadTrajectoryModifier` into the returned :py:class:`~ovito.pipeline.Pipeline`
        to also load the trajectory data.
    """
    
    # Determine the file's format.
    importer = FileImporter.autodetect_format(ovito.dataset, location)
    if not importer:
        raise RuntimeError("Could not detect the file format. The format might not be supported.")
    
    # Forward user parameters to the file importer object.
    for key in params:
        if not hasattr(importer, key):
            raise RuntimeError("Invalid keyword parameter. Object %s doesn't have a parameter named '%s'." % (repr(importer), key))
        importer.__setattr__(key, params[key])

    # Import data.
    if not importer.import_file(location, ImportMode.AddToScene, False):
        raise RuntimeError("Operation has been canceled by the user.")

    # Get the newly created ObjectNode.
    node = ovito.dataset.selected_node
    if not isinstance(node, Pipeline):
        raise RuntimeError("File import failed. Nothing was imported.")
    
    try:
        # Block execution until file is loaded.
        state = node.evaluate_pipeline(node.dataset.anim.time)

        # Raise exception if error occurs during loading.
        if state.status.type == PipelineStatus.Type.Error:
            raise RuntimeError(state.status.text)
    except:
        # Delete newly created scene node when import failed.
        node.delete()
        raise
    
    # Do not add node to scene by default.
    node.remove_from_scene()
    
    return node    