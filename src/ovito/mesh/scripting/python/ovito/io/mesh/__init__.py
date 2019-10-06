# Load dependencies
import ovito.io
import ovito.io.stdobj

# Load the native code module
from ovito.plugins.MeshPython import VTKTriangleMeshExporter

# Register export formats.
ovito.io.export_file._formatTable["vtk/trimesh"] = VTKTriangleMeshExporter
