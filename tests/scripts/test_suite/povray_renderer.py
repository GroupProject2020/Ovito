import ovito
from ovito.io import import_file
from ovito.vis import *

import sys
if "ovito.plugins.POVRay" not in sys.modules: sys.exit()

test_data_dir = "../../files/"
node1 = import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
node1.add_to_scene()
node1.source.particles['Position'].vis.radius = 0.3

renderer = POVRayRenderer(show_window = False)

print("POV-Ray executable:", renderer.povray_executable)
print("quality_level:", renderer.quality_level)
print("antialiasing:", renderer.antialiasing)
print("show_window:", renderer.show_window)
print("radiosity:", renderer.radiosity)
print("radiosity_raycount:", renderer.radiosity_raycount)
print("depth_of_field:", renderer.depth_of_field)
print("focal_length:", renderer.focal_length)
print("aperture:", renderer.aperture)
print("blur_samples:", renderer.blur_samples)

ovito.dataset.viewports.active_vp.render_image(size = (100,100), renderer = renderer)
