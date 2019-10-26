import sys

# This is the ovito.plugins Python package. It hosts the C++ extension modules of OVITO.

# The C++ extension modules are, however, located in a different directory when OVITO is installed
# as an application. For the time being, we use hardcoded, relative paths to find them.
#
# Platform-dependent paths where this Python module is located:
#   Linux:   lib/ovito/plugins/python/ovito/plugins/
#   Windows: plugins/python/ovito/plugins/
#   macOS:   Ovito.app/Contents/Resources/python/ovito/plugins/
#
# Platform-dependent paths where the native C++ shared libraries are located:
#   Linux:   lib/ovito/plugins/
#   Windows: plugins/
#   macOS:   Ovito.app/Contents/PlugIns/
#

if not hasattr(sys, '__OVITO_BUILD_MONOLITHIC'):
    # If the OVITO plugins are present as shared libraries, we need to specify
    # the path where they are found:
    if sys.platform.startswith('darwin'):  # macOS
        if __path__[0].endswith("/Resources/python/ovito/plugins"):
            __path__[0] += "/../../../../PlugIns"
    elif sys.platform.startswith('win32'):  # Windows
        if __path__[0].endswith("/plugins/python/ovito/plugins"):
            __path__[0] += "\\..\\..\\.."
    else:
        if __path__[0].endswith("/lib/ovito/plugins/python/ovito/plugins"):
            __path__[0] += "/../../.."

    # On Windows, extension modules for the Python interpreter have a .pyd file extension.
    # Our OVITO plugins, however, have the standard .dll extension. We therefore need to implement
    # a custom file entry finder and hook it into the import machinery of Python.
    # It specifically handles the OVITO plugin path and allows to load extension modules with .dll
    # extension instead of .pyd.
    if sys.platform.startswith('win32'):
        import importlib.machinery
        def OVITOPluginFinderHook(path):
            if path == __path__[0]:
                return importlib.machinery.FileFinder(path, (importlib.machinery.ExtensionFileLoader, ['.dll']))
            raise ImportError()
        sys.path_hooks.insert(0, OVITOPluginFinderHook)
else:
    # If the OVITO plugins were statically linked into the main executable,
    # make them loadable as built-in modules below the ovito.plugins package.
    import importlib.machinery
    def OVITOBuiltinPluginFinderHook(path):
        if path == __path__[0]:
            return importlib.machinery.BuiltinImporter()
        raise ImportError()
    sys.path_hooks.insert(0, OVITOBuiltinPluginFinderHook)

# Load all the PyQt5 modules first before the OVITO C++ modules get loaded.
# This ensures that the right Qt5 shared libraries get loaded when
# running in a system Python interpreter.
#
# Note: No need to load Qt5 modules that needed just by the OVITO desktop application.

import PyQt5
import PyQt5.QtCore
import PyQt5.QtNetwork
import PyQt5.QtGui
import PyQt5.QtXml

# Special handling for QtConcurrent module is required, because there is no corresponsing PyQt5 module.
# We need to locate the QtConcurrent shared library in the PyQt5 directory and load it ourselves.
if sys.platform.startswith('darwin'):  # macOS
    _QtConcurrent_library_path = PyQt5.__path__[0] + "/Qt/lib/QtConcurrent.framework/Versions/5/QtConcurrent"
else:
    _QtConcurrent_library_path = None
if _QtConcurrent_library_path is not None:
    import os.path
    if os.path.exists(_QtConcurrent_library_path):
        import ctypes
        ctypes.cdll.LoadLibrary(_QtConcurrent_library_path)
del _QtConcurrent_library_path