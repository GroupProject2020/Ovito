from ovito.vis import PythonViewportOverlay, Viewport

# Create a viewport:
viewport = Viewport(type = Viewport.Type.Top)

# The user-defined function that will paint on top of rendered images:
def render_some_text(args):
    args.painter.drawText(10, 10, "Hello world")

# Attach overlay function to viewport:
viewport.overlays.append(PythonViewportOverlay(function = render_some_text))