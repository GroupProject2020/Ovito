import ovito
from ovito.io import *
from ovito.vis import OpenGLRenderer

if not ovito.headless_mode:

    # Import a data file.
    node = import_file("../files/CFG/shear.void.120.cfg")
    node.add_to_scene()

    renderer = OpenGLRenderer()

    print("Parameter defaults:")
    print("  antialiasing_level: {}".format(renderer.antialiasing_level))
    renderer.antialiasing_level = 2

    ovito.scene.viewports.active_vp.render_image(size = (100,100), renderer = renderer)
