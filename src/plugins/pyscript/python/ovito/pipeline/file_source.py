# Load the native modules and other dependencies.
from ..plugins.PyScript import FileImporter, FileSource, PipelineStatus
from ..data import DataCollection

# Make FileSource a DataCollection.
DataCollection.registerDataCollectionType(FileSource)
assert(issubclass(FileSource, DataCollection))

# Implementation of FileSource.load() method:
def _FileSource_load(self, location, **params):
    """ Loads a new external data file into this pipeline source object.
    
        The function accepts additional keyword arguments that are forwarded to the format-specific file importer.
        See the documentation of the :py:func:`~ovito.io.import_file` function for more information.

        :param str location: The local file or remote *sftp://* URL to load.
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
    if self.status.type == PipelineStatus.Type.Error:
        raise RuntimeError(self.status.text)
    
FileSource.load = _FileSource_load

# Implementation of FileSource.source_path property, which returns or sets the currently loaded file path.
def _get_FileSource_source_path(self, _originalGetterMethod = FileSource.source_path):
    """ The path or URL of the imported file or file sequence. Unlike the path return by :py:attr:`.loaded_file`, the source path may contain a '*' wildcard character when a file sequence has been imported. """
    return _originalGetterMethod.__get__(self)
def _set_FileSource_source_path(self, url):
    """ Sets the URL of the file referenced by this FileSource. """
    self.setSource(url, self.importer, False) 
FileSource.source_path = property(_get_FileSource_source_path, _set_FileSource_source_path)
