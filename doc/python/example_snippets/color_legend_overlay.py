import ovito
from ovito.io import import_file
from ovito.vis import ColorLegendOverlay
from ovito.modifiers import ColorCodingModifier
from PyQt5.QtCore import Qt

# Prepare a data pipeline containing a ColorCodingModifier:
pipeline = import_file("input/simulation.dump")
color_mod = ColorCodingModifier(particle_property = 'peatom')
pipeline.modifiers.append(color_mod)
pipeline.add_to_scene()

# Configure the viewport overlay and link it to the ColorCodingModifier:
overlay = ColorLegendOverlay(
    modifier = color_mod,
    title = 'Potential energy per atom:', 
    alignment = Qt.AlignLeft ^ Qt.AlignTop,
    orientation = Qt.Vertical,
    offset_y = -0.04,
    font_size = 0.12,
    format_string = '%.2f eV')

# Attach the overlay to the Viewport that is going to be used for rendering:
viewport = ovito.dataset.viewports.active_vp
viewport.overlays.append(overlay)