test_flag = False

# >>>>>>>>> snippet start here >>>>>>>>>>>>>>>
from ovito.vis import ParticlesVis
from PyQt5.QtCore import *
from PyQt5.QtGui import *

def render(args):
    
    # Get output data collection of first scene pipeline.
    data = args.scene.pipelines[0].compute()
    positions = data.particles.positions
    pindex = 0 # The index of the particle to be highlighted
    
    # Project center point of particle.
    xy = args.project_point(positions[pindex])
    if xy is None: return
    
    # Determine display radius of the particle.
    radius = 0.0
    if 'Radius' in data.particles:
        radius = data.particles['Radius'][pindex]
    if radius <= 0 and data.particles.particle_types:
        particle_type = data.particles.particle_types[pindex]
        radius = data.particles.particle_types.type_by_id(particle_type).radius
    if radius <= 0:
        radius = positions.vis.radius

    # Calculate screen-space size of the particle in pixels.
    screen_radius = args.project_size(positions[pindex], radius)

    # Draw a dashed circle around the particle.
    pen = QPen(Qt.DashLine)
    pen.setWidth(3)
    pen.setColor(QColor(0,0,255))
    args.painter.setPen(pen)	
    args.painter.drawEllipse(QPointF(xy[0], xy[1]), screen_radius, screen_radius)
    
    # Draw an arrow pointing at the particle.
    arrow_shape = QPolygonF()
    arrow_shape.append(QPointF(0,0))
    arrow_shape.append(QPointF(10,10))
    arrow_shape.append(QPointF(10,5))
    arrow_shape.append(QPointF(40,5))
    arrow_shape.append(QPointF(40,-5))
    arrow_shape.append(QPointF(10,-5))
    arrow_shape.append(QPointF(10,-10))
    args.painter.setPen(QPen())
    args.painter.setBrush(QBrush(QColor(255,0,0)))
    args.painter.translate(QPointF(xy[0], xy[1]))
    args.painter.rotate(-45.0)
    args.painter.translate(QPointF(screen_radius,0))
    args.painter.scale(2,2)
    args.painter.drawPolygon(arrow_shape)

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