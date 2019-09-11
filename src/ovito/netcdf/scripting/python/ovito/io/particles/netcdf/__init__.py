# Load dependencies
import ovito.io.particles

# Load the native code module.
from ovito.plugins.NetCDFPluginPython import AMBERNetCDFExporter

# Register export formats.
ovito.io.export_file._formatTable["netcdf/amber"] = AMBERNetCDFExporter
