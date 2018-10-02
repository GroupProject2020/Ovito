import ovito
from ovito.io import *
from ovito.vis import *
import os
import os.path

pipeline = import_file("../../files/LAMMPS/animation.dump.gz")
pipeline.add_to_scene()

vp = Viewport()
vp.zoom_all()

output_file = "_movie.avi"

if os.path.isfile(output_file):
    os.remove(output_file)
assert(not os.path.isfile(output_file))

if ovito.headless_mode: 
    renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)
else:
    renderer = None
vp.render_anim(output_file, size=(64,64), renderer=renderer, fps=26)

assert(os.path.isfile(output_file))
os.remove(output_file)
