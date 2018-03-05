import ovito
from ovito.io import *
from ovito.vis import *

# Import a data file.
pipeline = import_file("../../files/CFG/shear.void.120.cfg")
pipeline.add_to_scene()

print("ovito.headless_mode=", ovito.headless_mode)

settings = RenderSettings(size = (20,20))
if ovito.headless_mode: 
    renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)
else:
    renderer = None

vp = Viewport()
vp.render_image(background = (0.8,0.8,1.0), renderer = renderer, size=(10,10))
