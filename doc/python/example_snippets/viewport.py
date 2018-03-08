from ovito.io import import_file
from ovito.vis import Viewport, TachyonRenderer

pipeline = import_file('input/simulation.dump')
pipeline.add_to_scene()

vp = Viewport(type = Viewport.Type.Ortho, camera_dir = (2, 1, -1))
vp.zoom_all()
vp.render_image(filename='output/simulation.png', 
                size=(320, 240), 
                renderer=TachyonRenderer())