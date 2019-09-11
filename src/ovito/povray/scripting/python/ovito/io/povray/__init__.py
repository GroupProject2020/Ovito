# Load dependencies
import ovito.io

# Load the native code module
from ovito.plugins.POVRayPython import POVRayExporter

# Register export formats.
ovito.io.export_file._formatTable["povray"] = POVRayExporter
