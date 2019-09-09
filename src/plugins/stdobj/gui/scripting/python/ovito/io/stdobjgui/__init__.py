# Load dependencies
import ovito.io
import ovito.io.stdobj

# Load the native module.
from ovito.plugins.StdObjGuiPython import DataSeriesPlotExporter

# Register export formats.
ovito.io.export_file._formatTable["plot/series"] = DataSeriesPlotExporter
