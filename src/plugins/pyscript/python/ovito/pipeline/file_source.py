# Load the native modules and other dependencies.
from ..plugins.PyScript import FileImporter, FileSource, PipelineStatus
from ..data import DataCollection
import sys
import ovito
try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

# Implementation of FileSource.load() method:
def _FileSource_load(self, location, **params):
    """ Sets a new input file, from which this data source will retrieve its data from.
    
        The function accepts additional keyword arguments, which are forwarded to the format-specific file importer.
        For further information, please see the documentation of the :py:func:`~ovito.io.import_file` function,
        which has the same function interface as this method.

        :param str location: The local file(s) or remote *sftp://* URL to load.
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
    importer = FileImporter.autodetect_format(self.dataset, first_location)
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
    if not self.set_source(location_list, importer, False):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Block execution until data has been loaded. 
    if not self.wait_until_ready(self.dataset.anim.time):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Raise Python error if loading failed.
    if self.status.type == PipelineStatus.Type.Error:
        raise RuntimeError(self.status.text)

    # Block until list of animation frames has been loaded
    if not self.wait_for_frames_list():
        raise RuntimeError("Operation has been canceled by the user.")
    
FileSource.load = _FileSource_load

# Implementation of FileSource.source_path property.
def _get_FileSource_source_path(self):
    """ This read-only attribute returns the path(s) or URL(s) of the file(s) where this :py:class:`!FileSource` retrieves its input data from. 
        You can change the source path by calling :py:meth:`.load`. """
    path_list = self.get_source_paths()
    if len(path_list) == 1: return path_list[0]
    elif len(path_list) == 0: return ""
    else: return path_list
FileSource.source_path = property(_get_FileSource_source_path)

def _FileSource_compute(self, frame = None):
    """ Requests data from this data source. The :py:class:`!FileSource` will load it from the external file if needed.

        The optional *frame* parameter determines the frame to retrieve, which must be in the range 0 through (:py:attr:`.num_frames`-1).
        If no frame number is specified, the current animation position is used (frame 0 by default).

        The :py:class:`!FileSource` uses a caching mechanism to keep the data for one or more frames in memory. Thus, invoking :py:meth:`!compute`
        repeatedly to retrieve the same frame will typically be very fast.

        Note: The returned :py:class:`~ovito.data.DataCollection` contains data objects that are owned by the :py:class:`!FileSource`.
        That means it is not safe to modify these objects as this would lead to unexpected side effects. 
        You should always use the :py:meth:`DataCollection.copy_if_needed() <ovito.data.DataCollection.copy_if_needed>` method
        to make a copy of the data objects that you want to modify. See the :py:class:`~ovito.data.DataCollection` class
        for more information.

        :param int frame: The animation frame number at which the source should be evaluated. 
        :return: A :py:class:`~ovito.data.DataCollection` containing the loaded data.
    """
    if frame is not None:
        time = self.source_frame_to_anim_time(frame)
    else:
        time = self.dataset.anim.time

    state = self._evaluate(time)
    if state.status.type == PipelineStatus.Type.Error:
        raise RuntimeError("Data source evaluation failed: %s" % state.status.text)
    assert(state.status.type != PipelineStatus.Type.Pending)

    # Wait for worker threads to finish.
    # This is to avoid warning messages 'QThreadStorage: Thread exited after QThreadStorage destroyed'
    # during Python program exit.
    if not hasattr(sys, '__OVITO_BUILD_MONOLITHIC'):
        import PyQt5.QtCore
        PyQt5.QtCore.QThreadPool.globalInstance().waitForDone(0)

    return state.mutable_data

FileSource.compute = _FileSource_compute