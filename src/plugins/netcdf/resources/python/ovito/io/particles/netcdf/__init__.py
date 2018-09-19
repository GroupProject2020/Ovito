# Load dependencies
import ovito.io.particles

# Load the native code module.
import ovito.plugins.NetCDFPlugin

# Register export formats.
ovito.io.export_file._formatTable["netcdf/amber"] = ovito.plugins.NetCDFPlugin.AMBERNetCDFExporter
