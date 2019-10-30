# Load dependencies
import ovito.io.stdobj
import ovito.io.mesh
import ovito.io.grid
import ovito.io.stdmod
import ovito.io.particles

# Load the native code modules
from ovito.plugins.CrystalAnalysisPython import CAExporter, VTKDislocationsExporter

# Register export formats.
ovito.io.export_file._formatTable["ca"] = CAExporter
ovito.io.export_file._formatTable["vtk/disloc"] = VTKDislocationsExporter
