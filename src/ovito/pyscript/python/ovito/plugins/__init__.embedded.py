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
    if sys.platform.startswith('darwin'):
        # macOS
        __path__[0] += "/../../../../PlugIns"
    elif sys.platform.startswith('win32'):
        # Windows
        __path__[0] += "\\..\\..\\.."
    else:
        # Linux
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
