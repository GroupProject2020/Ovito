# Load dependencies
import ovito.io
import ovito.io.stdobj
import ovito.io.stdmod

# Load the native code module
from ovito.plugins.MeshPython import VTKFileImporter, VTKTriangleMeshExporter

# Register export formats.
ovito.io.export_file._formatTable["vtk/trimesh"] = VTKTriangleMeshExporter
