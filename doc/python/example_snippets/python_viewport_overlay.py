from ovito.vis import PythonViewportOverlay, Viewport

# The user-defined function that paints on top of the rendered image:
def render_overlay(painter, **args):
    painter.drawText(10, 10, "Hello world")

# Create the overlay.
overlay = PythonViewportOverlay(function = render_overlay)

# Attach overlay to a newly created viewport.
viewport = Viewport(type = Viewport.Type.Top)
viewport.overlays.append(overlay)
