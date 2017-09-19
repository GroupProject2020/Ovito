# Load dependencies
import ovito.io

# Load the native code module
from ovito.plugins.Mesh import VTKFileImporter, VTKTriangleMeshExporter

# Register export formats.
ovito.io.export_file._formatTable["vtk/trimesh"] = VTKTriangleMeshExporter
