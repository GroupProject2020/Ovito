from ovito.vis import Viewport
from PyQt5.QtGui import QPainter

# Render an image of the three-dimensional scene:
vp = Viewport(type=Viewport.Type.Ortho, camera_dir=(2, 1, -1))
vp.zoom_all()
image = vp.render_image(size=(320,240))

# Paint on top of the rendered image using Qt's drawing functions:
painter = QPainter(image)
painter.drawText(10, 20, "Hello world!")
del painter

# Save image to disk:
image.save("output/image.png")