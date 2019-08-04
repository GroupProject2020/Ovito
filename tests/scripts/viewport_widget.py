# Note: The Ovito modules should generally be imported before the PyQt5 modules,
# in order to avoid loading the wrong copies of the Qt shared libraries into
# memory.
from ovito.vis import Viewport
from ovito.pipeline import StaticSource, Pipeline
from ovito.data import DataCollection, SimulationCell, Particles
from ovito.modifiers import CreateBondsModifier

import sys
from PyQt5.QtWidgets import QApplication, QWidget

def create_ovito_pipeline():
    # Insert a SimulationCell object into a new data collection:
    data = DataCollection()
    cell = SimulationCell(pbc = (False, False, False))
    cell.vis.line_width = 0.1
    with cell:
        cell[:,0] = (5,0,0)
        cell[:,1] = (0,5,0)
        cell[:,2] = (0,0,5)
    data.objects.append(cell)

    # Create a Particles object containing three particles:
    particles = Particles()
    particles.create_property('Position', data=[[1,1,1],[4,4,1],[1,4,4]])
    particles.create_property('Color', data=[[1,0,0],[0,1,0],[0,0,1]])
    data.objects.append(particles)

    # Create a new Pipeline with a StaticSource as data source:
    pipeline = Pipeline(source = StaticSource(data = data))

    # Apply a modifier:
    pipeline.modifiers.append(CreateBondsModifier(cutoff = 6.0))

    return pipeline

# Make sure we are not running within the context of an OVITO session.
# This script doesn't work when executed with the ovitos interpreter application.
if 'ovitos' in sys.executable:
    print("Warning: This Python script should not be run using the 'ovitos' interpreter. Please use a normal Python interpreter instead.")
    sys.exit(0)

# Create the global Qt application object:
app = QApplication(sys.argv)

# Add a data pipeline to the current scene, which will show up in the viewport:
pipeline = create_ovito_pipeline()
pipeline.add_to_scene()

# Something that the OVITO GUI does automatically:
# Here we have to explicitly evaluate the data pipeline in order to update the rendered data.
pipeline.compute()

# Create a viewport:
vp = Viewport(type = Viewport.Type.Ortho, camera_dir = (-1, 2, -1))

# Create a GUI widget for the viewport:
vpwin = vp.create_widget(None)
vpwin.resize(500, 300)
vp.zoom_all()
vpwin.setWindowTitle('OVITO Viewport Demo')
vpwin.show()

# Enter the event loop (commented out here, because this is an automated test script):
# sys.exit(app.exec_())
