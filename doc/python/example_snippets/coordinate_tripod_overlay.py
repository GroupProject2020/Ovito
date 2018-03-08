from ovito.vis import CoordinateTripodOverlay, Viewport
from PyQt5 import QtCore

# Create the overlay.
tripod = CoordinateTripodOverlay()
tripod.size = 0.07
tripod.alignment = QtCore.Qt.AlignRight ^ QtCore.Qt.AlignBottom

# Attach overlay to a newly created viewport.
viewport = Viewport(type=Viewport.Type.Perspective, camera_dir=(1,2,-1))
viewport.overlays.append(tripod)