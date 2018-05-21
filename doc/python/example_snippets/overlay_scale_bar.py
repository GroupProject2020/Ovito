test_flag = False
import ovito
# >>>>>>>>> snippet start here >>>>>>>>>>>>>>>
from PyQt5.QtCore import *
from PyQt5.QtGui import *

# Parameters:
bar_length = 40   # Simulation units (e.g. Angstroms)
bar_color = QColor(0,0,0)
label_text = "{} nm".format(bar_length/10)
label_color = QColor(255,255,255)

# This function is called by OVITO on every viewport update.
def render(args):
    if args.is_perspective: 
        raise Exception("This overlay only works with non-perspective viewports.")
        
    # Compute length of bar in screen space
    screen_length = args.project_size((0,0,0), bar_length)

    # Define geometry of bar in screen space
    height = 0.07 * args.painter.window().height()
    margin = 0.02 * args.painter.window().height()
    rect = QRectF(margin, margin, screen_length, height)

    # Render bar rectangle
    args.painter.fillRect(rect, bar_color)

    # Render text label
    font = args.painter.font()
    font.setPixelSize(height)
    args.painter.setFont(font)
    args.painter.setPen(QPen(label_color))
    args.painter.drawText(rect, Qt.AlignCenter, label_text)

# <<<<<<<<<<<<< snippet ends here <<<<<<<<<<<<<
    global test_flag
    test_flag = True # Indicate to the driver script that this function has been executed

from ovito.vis import PythonViewportOverlay, Viewport, TachyonRenderer
from ovito.io import import_file

pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()
viewport = Viewport(type = Viewport.Type.Top)
viewport.overlays.append(PythonViewportOverlay(function = render))
viewport.render_image(filename='output/simulation.png', 
                size=(40, 40), 
                renderer=TachyonRenderer())
assert(test_flag)