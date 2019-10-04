# Load dependencies
import ovito.io
import ovito.io.mesh
import ovito.io.grid
import ovito.io.stdobj
import ovito.io.stdmod

# Load the native code modules
from ovito.plugins.ParticlesPython import LAMMPSDataImporter
from ovito.plugins.ParticlesPython import (LAMMPSDumpExporter, LAMMPSDataExporter, IMDExporter, POSCARExporter, XYZExporter, FHIAimsExporter, GSDExporter)

# Register export formats.
ovito.io.export_file._formatTable["lammps/dump"] = LAMMPSDumpExporter
ovito.io.export_file._formatTable["lammps/data"] = LAMMPSDataExporter
ovito.io.export_file._formatTable["imd"] = IMDExporter
ovito.io.export_file._formatTable["vasp"] = POSCARExporter
ovito.io.export_file._formatTable["xyz"] = XYZExporter
ovito.io.export_file._formatTable["fhi-aims"] = FHIAimsExporter
ovito.io.export_file._formatTable["gsd/hoomd"] = GSDExporter

# For backward compatibility with OVITO 2.9.0:
ovito.io.export_file._formatTable["lammps_dump"] = LAMMPSDumpExporter
ovito.io.export_file._formatTable["lammps_data"] = LAMMPSDataExporter

# Implementation of the LAMMPSDataImporter.atom_style property:
def _get_LAMMPSDataImporter_atom_style(self):
    return str(self._atom_style)
def _set_LAMMPSDataImporter_atom_style(self, style):
    if str(style) not in LAMMPSDataImporter.LAMMPSAtomStyle.__dict__:
        raise KeyError("'%s' is not a valid LAMMPS atom style supported by OVITO. Must be one of %s." %
            (style, [s for s in dir(LAMMPSDataImporter.LAMMPSAtomStyle) if not s.startswith('_')]))
    self._atom_style = LAMMPSDataImporter.LAMMPSAtomStyle.__dict__[str(style)]
LAMMPSDataImporter.atom_style = property(_get_LAMMPSDataImporter_atom_style, _set_LAMMPSDataImporter_atom_style)

# Implementation of the LAMMPSDataExporter.atom_style property:
def _get_LAMMPSDataExporter_atom_style(self):
    return str(self._atom_style)
def _set_LAMMPSDataExporter_atom_style(self, style):
    if str(style) not in LAMMPSDataImporter.LAMMPSAtomStyle.__dict__:
        raise KeyError("'%s' is not a valid LAMMPS atom style supported by OVITO. Must be one of %s." %
            (style, [s for s in dir(LAMMPSDataImporter.LAMMPSAtomStyle) if not s.startswith('_')]))
    self._atom_style = LAMMPSDataImporter.LAMMPSAtomStyle.__dict__[str(style)]
LAMMPSDataExporter.atom_style = property(_get_LAMMPSDataExporter_atom_style, _set_LAMMPSDataExporter_atom_style)
