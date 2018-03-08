from ovito.vis import TextLabelOverlay, Viewport
from PyQt5 import QtCore

# Create the overlay:
overlay = TextLabelOverlay(
    text = 'Some text', 
    alignment = QtCore.Qt.AlignHCenter ^ QtCore.Qt.AlignBottom,
    offset_y = 0.1,
    font_size = 0.03,
    text_color = (0,0,0))

# Attach the overlay to a newly created viewport:
viewport = Viewport(type = Viewport.Type.Top)
viewport.overlays.append(overlay)