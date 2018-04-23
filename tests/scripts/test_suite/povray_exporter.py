import ovito
from ovito.io import import_file, export_file
from ovito.vis import *
import os

import sys
if "ovito.plugins.POVRay" not in sys.modules: sys.exit()

test_data_dir = "../../files/"

pipeline1 = import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
pipeline1.add_to_scene()
pipeline1.compute().particles['Position'].vis.shape = ParticlesVis.Shape.Square
pipeline1.compute().particles['Position'].vis.radius = 0.3
export_file(pipeline1, "_povray_export_test.pov", "povray")
assert(os.path.isfile("_povray_export_test.pov"))
os.remove("_povray_export_test.pov")
export_file(None, "_povray_export_test.pov", "povray")
assert(os.path.isfile("_povray_export_test.pov"))
os.remove("_povray_export_test.pov")
