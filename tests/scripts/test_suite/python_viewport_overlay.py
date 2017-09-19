import ovito
from ovito.io import *
from ovito.modifiers import *
from ovito.vis import *

import numpy

node = import_file("../../files/LAMMPS/animation.dump.gz")
node.modifiers.append(CommonNeighborAnalysisModifier())
node.add_to_scene()

vp = ovito.dataset.viewports.active_vp

settings = RenderSettings(size = (16,16))
if ovito.headless_mode:
    settings.renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)

def myrender(painter, **args):
    painter.drawText(10, 10, "Hello world")

new_overlay = PythonViewportOverlay(function = myrender)
vp.overlays.append(new_overlay)

assert(len(vp.overlays) == 1)
assert(vp.overlays[0] == new_overlay)
vp.render(settings)

def myrender2(painter, **args):
    print("The following render function will provoke an intentional error:")
    return this_is_an_intentional_error

overlay2 = PythonViewportOverlay(function = myrender2)
vp.overlays.append(overlay2)
assert(len(vp.overlays) == 2)

try:
    vp.render(settings)
    assert(False)
except:
    pass
    