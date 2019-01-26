import ovito
from ovito.io import *
from ovito.modifiers import *
from ovito.vis import *

import numpy

node = import_file("../files/LAMMPS/animation.dump.gz")
node.modifiers.append(CommonNeighborAnalysisModifier())
node.add_to_scene()

vp = Viewport()
vp.zoom_all()

if ovito.headless_mode:
    renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)
else:
    renderer = None

def myrender(args):
    args.painter.drawText(10, 10, "Hello world")

new_overlay = PythonViewportOverlay(function = myrender)
new_overlay.behind_scene = True
vp.overlays.append(new_overlay)

assert(len(vp.overlays) == 1)
assert(vp.overlays[0] == new_overlay)
vp.render_image(renderer=renderer, size=(10,10))

def myrender2(args):
    print("Note: The following render function will generate an error intentionally:")
    return this_error_is_intentional

overlay2 = PythonViewportOverlay(function = myrender2)
vp.overlays.append(overlay2)
assert(len(vp.overlays) == 2)

try:
    vp.render_image(renderer=renderer, size=(10,10))
    assert(False)
except:
    pass
    
# Try again with a disabled overlay:
overlay2.enabled = False
vp.render_image(renderer=renderer, size=(10,10))

print("OK")