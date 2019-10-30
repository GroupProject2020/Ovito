# Load dependencies
import ovito.io.stdobj
import ovito.io.stdmod
import ovito.io.mesh

# Load the native code module
from ovito.plugins.GridPython import VTKVoxelGridExporter

# Register export formats.
ovito.io.export_file._formatTable["vtk/grid"] = VTKVoxelGridExporter
