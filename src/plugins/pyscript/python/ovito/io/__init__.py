""" This module contains functions and classes related to file input and output.

It primarily provides two high-level functions for reading and writing
external files:

    * :py:func:`import_file`
    * :py:func:`export_file`

In addition, the module contains the :py:class:`FileSource` class, which is a data source for OVITO's 
data pipeline system reading its input data from an external file.
"""

import ovito.data
import ovito.plugins.PyScript.Scene

# Load the native module.
from ..plugins.PyScript.IO import *

__all__ = ['import_file', 'export_file', 'FileSource']

def import_file(location, **params):
    """ This high-level function imports external data from a file. 
    
        This Python function corresponds to the *Load File* command in OVITO's
        user interface. The format of the imported file is automatically detected.
        However, depending on the file's format, additional keyword parameters may need to be supplied to 
        to specify how the data should be interpreted. These keyword parameters are documented below.
        
        :param str location: The file to import. This can be a local file path or a remote sftp:// URL.
        :returns: The :py:class:`~ovito.ObjectNode` that has been created for the imported data.

        The function creates and returns a new :py:class:`~ovito.ObjectNode`, which provides access the imported data
        or allows you to apply modifiers to it. 
        
        .. note:: 
        
           Note that the newly created :py:class:`~ovito.ObjectNode` is not automatically inserted into the three-dimensional scene. 
           That means it won't appear in the interactive viewports of OVITO or in rendered images.
           However, you can subsequently insert the node into the scene by calling the :py:meth:`~ovito.ObjectNode.add_to_scene` method on it.
        
        Sometimes it may be desirable to reuse an existing :py:class:`~ovito.ObjectNode`. For example if you have already set up a 
        modification pipeline and just want to replace the input data with a different file. In this case you can
        call :py:meth:`node.source.load(...) <ovito.io.FileSource.load>` instead on the existing :py:class:`~ovito.ObjectNode`
        to select another input file while keeping the applied modifiers.
        
        **File columns**
        
        When importing XYZ files or *binary* LAMMPS dump files, the mapping of file columns 
        to OVITO's particle properties must be specified using the ``columns`` keyword parameter::
        
            import_file("file.xyz", columns = 
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
        
        The length of the list must match the number of columns in the input file. 
        See the list of :ref:`particle properties <particle-types-list>` for standard property names. You can also specify
        a custom property name, in which case a user-defined particle property with that name is created from the corresponding file column.
        For vector properties, the component must be appended to the property base name as demonstrated for the ``Position`` property in the example above. 
        To skip a file column during import, specify ``None`` instead of a property name at the corresponding position in the columns list.
        
        For *text-based* LAMMPS dump files it is also possible to explicitly specify a file column mapping using the
        ``columns`` keyword. Overriding the default mapping can be useful, for example, if the file columns containing the particle positions
        do not have the standard names ``x``, ``y``, and ``z`` (e.g. when reading time-averaged atomic positions computed by LAMMPS). 
        
        **File sequences**
        
        You can import a sequence of files by passing a filename containing a ``*`` wildcard character to :py:func:`!import_file`. There may be 
        only one ``*`` in the filename (and not in a directory name). The wildcard matches only to numbers in a filename.
        
        OVITO scans the directory and imports all matching files that belong to the sequence. Note that OVITO only loads the first file into memory though.
        
        The length of the imported time series is reported by the :py:attr:`~ovito.io.FileSource.num_frames` field of the :py:class:`~ovito.io.FileSource`
        class and is also reflected by the global :py:class:`~ovito.anim.AnimationSettings` object. You can step through the frames of the animation
        sequence as follows:
        
        .. literalinclude:: ../example_snippets/import_access_animation_frames.py
        
        **Multi-timestep files**
        
        Some file formats can store multiple frames in a single file. OVITO cannot know in some cases (e.g. XYZ and LAMMPS dump formats)
        that the file contains multiple frames (because, by default, reading the entire file is avoided for performance reasons). 
        Then it is necessary to explicitly tell OVITO to scan the entire file and load a sequence of frames by supplying the ``multiple_frames`` 
        option:: 
        
            node = import_file("file.dump", multiple_frames = True)
            
        You can then step through the contained frames in the same way as for sequences of files.

        **LAMMPS atom style**
        
        When trying to load a LAMMPS data file which is using an atom style other than "atomic", the atom style must be explicitly
        specified using the ``atom_style`` keyword parameter. The following LAMMPS atom styles are currently supported by
        OVITO: ``angle``, ``atomic``, ``body``, ``bond``, ``charge``, ``dipole``, ``full``, ``molecular``.
            
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
    if not isinstance(node, ovito.ObjectNode):
        raise RuntimeError("File import failed. Nothing was imported.")
    
    try:
        # Block execution until file is loaded.
        # Raise exception if error occurs during loading, or if canceled by the user.
        if not node.wait(signalError = True):
            raise RuntimeError("Operation has been canceled by the user.")
    except:
        # Delete newly created scene node when import failed.
        node.delete()
        raise
    
    # Do not add node to scene by default.
    node.remove_from_scene()
    
    return node    

def _FileSource_load(self, location, **params):
    """ Loads a new external file into this data source object.
    
        The function auto-detects the format of the file.
        
        The function accepts additional keyword arguments that are forwarded to the format-specific file importer.
        See the documentation of the :py:func:`import_file` function for more information on this.

        :param str location: The local file or remote sftp:// URL to load.
    """

    # Determine the file's format.
    importer = FileImporter.autodetect_format(self.dataset, location)
    if not importer:
        raise RuntimeError("Could not detect the file format. The format might not be supported.")

    # Re-use existing importer if compatible.
    if self.importer and type(self.importer) == type(importer):
        importer = self.importer
        
    # Forward user parameters to the importer.
    for key in params:
        if not hasattr(importer, key):
            raise RuntimeError("Importer object %s does not have an attribute '%s'." % (importer, key))
        importer.__setattr__(key, params[key])

    # Load new data file.
    if not self.set_source(location, importer, False):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Block execution until data has been loaded. 
    if not self.wait_until_ready(self.dataset.anim.time):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Raise Python error if loading failed.
    if self.status.type == ovito.plugins.PyScript.Scene.PipelineStatus.Type.Error:
        raise RuntimeError(self.status.text)
    
FileSource.load = _FileSource_load

# Implement the 'sourceUrl' property of FileSource, which returns or sets the currently loaded file path.
def _get_FileSource_source_path(self, _originalGetterMethod = FileSource.source_path):
    """ The path or URL of the imported file or file sequence. Unlike the path return by :py:attr:`.loaded_file`, the source path may contain a '*' wildcard character when a file sequence has been imported. """
    return _originalGetterMethod.__get__(self)
def _set_FileSource_source_path(self, url):
    """ Sets the URL of the file referenced by this FileSource. """
    self.setSource(url, self.importer, False) 
FileSource.source_path = property(_get_FileSource_source_path, _set_FileSource_source_path)

def export_file(node, file, format, **params):
    """ High-level function that exports the results of the data pipeline to a file.
        See the :ref:`file_output_overview` section for an overview.
    
        :param node: The object node that provides the data to be exported.
        :type node: :py:class:`~ovito.scene.ObjectNode` 
        :param str file: The output file path.
        :param str format: The type of file to write:
        
                            * ``"txt"`` -- A text file with global quantities computed by OVITO (see below)
                            * ``"lammps_dump"`` -- LAMMPS text-based dump format
                            * ``"lammps_data"`` -- LAMMPS data format
                            * ``"imd"`` -- IMD format
                            * ``"vasp"`` -- POSCAR format
                            * ``"xyz"`` -- XYZ format
                            * ``"fhi-aims"`` -- FHI-aims format
                            * ``"ca"`` -- `Text-based format for storing dislocation lines <../../particles.modifiers.dislocation_analysis.html#particles.modifiers.dislocation_analysis.fileformat>`_
                            * ``"povray"`` -- POV-Ray scene format
        
        The function first evaluates the modification pipeline of the given :py:class:`~ovito.ObjectNode` to obtain the data to be exported.
        This means it is not necessary to call :py:meth:`ObjectNode.compute() <ovito.ObjectNode.compute>` before calling
        :py:func:`!export_file`, but it doesn't hurt either.

        The ``format`` parameter determines the type of file written by the export function; the filename suffix is ignored.
        However, for files that end in ``.gz``, automatic gzip compression is activated if the file format is text-based.
        
        Depending on the selected format, additional keyword arguments must be provided:
       
        **File columns**
        
        When writing files in one of the formats *lammps_dump*, *xyz*, or *imd*, you must specify the set of particle properties to be exported 
        using the ``columns`` keyword parameter::
        
            export_file(node, "output.xyz", "xyz", columns= 
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"]
            )
            
        See the list of standard :ref:`particle properties <particle-types-list>` for valid names. Additionaly, user-defined properties
        can be listed if they exist in the output data. For vector properties, the component name must be appended to the base name as 
        demonstrated for the ``Position`` property in the example above. 
        
        **Exporting several simulation frames**
        
        By default, only the current animation frame (given by the :py:attr:`~ovito.anim.AnimationSettings.current_frame` global variable) is exported.
        To export a different frame, pass the ``frame`` keyword parameter to the :py:func:`!export_file` function. 
        Alternatively, you can export all frames of the current animation sequence at once by passing ``multiple_frames=True``. Refined
        control of the exported frames is possible using the keyword arguments ``start_frame``, ``end_frame``, and ``every_nth_frame``.
        
        The *lammps_dump* and *xyz* file formats can store multiple frames in a single output file. For all other formats, or
        if you intentionally want to generate one file per frame, you must pass a wildcard filename to :py:func:`!export_file`.
        This filename must contain exactly one ``*`` character as in the following example, which will be replaced with the
        animation frame number::

            export_file(node, "output.*.dump", "lammps_dump", multiple_frames=True)
            
        The above call is equivalent to the following Python loop::
        
            for i in range(node.source.num_frames):
                export_file(node, "output.%i.dump" % i, "lammps_dump", frame=i)
       
        **LAMMPS atom style**
        
        When writing files in the *lammps_data* format, the LAMMPS atom style "atomic" is used by default. If you want to create 
        a data file with a different atom style, the style can be selected using the ``atom_style`` keyword parameter::
        
            export_file(node, "output.data", "lammps_data", atom_tyle="bond")
        
        The following `LAMMPS atom styles <http://lammps.sandia.gov/doc/atom_style.html>`_ are currently supported by OVITO:
        ``angle``, ``atomic``, ``bond``, ``charge``, ``dipole``, ``full``, ``molecular``, ``sphere``.
        
        **Global attributes**
        
        The *txt* file format allows you to export global quantities computed by OVITO's data pipeline to a text file. 
        For example, to write out the number of FCC atoms identified by a :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier`
        as a function of simulation time, one would do the following::
        
            export_file(node, "data.txt", "txt", 
                columns=["Timestep", "CommonNeighborAnalysis.counts.FCC"], 
                multiple_frames=True)
            
        See the documentation of each analysis modifier to find out which global quantities it 
        computes. At runtime, you can determine which :py:attr:`~ovito.data.DataCollection.attributes` are available for export as follows::
        
            print(node.compute().attributes)

    """

    # Determine the animation frame to be exported.    
    if 'frame' in params:
        frame = int(params['frame'])
        time = ovito.dataset.anim.frame_to_time(frame)
        params['multiple_frames'] = True
        params['start_frame'] = frame
        params['end_frame'] = frame
        del params['frame']
    else:
        time = ovito.dataset.anim.time    

    # Look up the exporter class for the selected format.
    if not format in export_file._formatTable:
        raise RuntimeError("Unknown output file format: %s" % format)
    
    # Create an instance of the exporter class.
    exporter = export_file._formatTable[format](params)
    
    # Pass function parameters to exporter object.
    exporter.output_filename = file
    
    # Detect wildcard filename.
    if '*' in file:
        exporter.wildcard_filename = file
        exporter.use_wildcard_filename = True
    
    # Ensure the data to be exported is available.
    if isinstance(node, ovito.ObjectNode):
        if not node.wait(time = time):
            raise RuntimeError("Operation has been canceled by the user.")
        # Pass objects to exporter object.
        exporter.set_node(node)
    elif node is None:
        exporter.select_standard_output_data()
    else:
        raise ValueError("Invalid node parameter.")

    # Export data.
    if not exporter.export_nodes(ovito.task_manager):
        raise RuntimeError("Operation has been canceled by the user.")

# This is the table of export formats used by the export_file() function
# to look up the right exporter class for a file format.
# Plugins can register their exporter class by inserting a new entry in this dictionary.
export_file._formatTable = {}
export_file._formatTable["txt"] = AttributeFileExporter
