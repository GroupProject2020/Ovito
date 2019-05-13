from ovito.io import *
from ovito.vis import *
import ovito
from PyQt5 import QtCore
from PyQt5 import QtGui
import numpy as np

# Import a data file.
pipeline = import_file("../files/CFG/shear.void.120.cfg")
pipeline.add_to_scene()

settings = RenderSettings(size = (20,20))
settings.renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)

vp = Viewport()
vp.type = Viewport.Type.PERSPECTIVE
vp.type = Viewport.Type.Perspective
vp.camera_pos = (350, -450, 450)
vp.camera_dir = (-100, -50, -50)
print(vp.fov)
print(vp.type)
vp.zoom_all()

vp.camera_pos = (0, 0, 450)
vp.camera_dir = (0, 0, -50)
vp.camera_up = (1,0,0)
print(vp.camera_tm)
assert(np.allclose(vp.camera_up, vp.camera_tm[:,1]))
vp.camera_up = (0,1,0)
print(vp.camera_tm)
assert(np.allclose(vp.camera_up, vp.camera_tm[:,1]))

overlay = TextLabelOverlay(
    text = 'Some text',
    alignment = QtCore.Qt.AlignHCenter ^ QtCore.Qt.AlignBottom,
    offset_y = 0.1,
    font_size = 0.03,
    text_color = (0,0,0))

vp.zoom_all()
vp.overlays.append(overlay)
vp.render(settings)
img = vp.render_image(renderer=settings.renderer, size=settings.size)
assert(img.hasAlphaChannel())
assert(img.pixel(0,0) == 0xffffffff)

img = vp.render_image(renderer=settings.renderer, size=settings.size, alpha=True)
assert(img.hasAlphaChannel())
assert(img.pixel(0,0) == 0)

for vp in ovito.scene.viewports:
    print(vp)