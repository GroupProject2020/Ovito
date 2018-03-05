from ovito.vis import Viewport
from ovito.io import import_file

pipeline = import_file('input/simulation.dump')
pipeline.add_to_scene()

vp = Viewport()
vp.type = Viewport.Type.Ortho
vp.camera_dir = (2, 1, -1)
vp.zoom_all()
vp.render_image(filename='output/simulation.png', size=(320, 240))