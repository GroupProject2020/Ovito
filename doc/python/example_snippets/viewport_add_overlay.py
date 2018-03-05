from ovito.vis import Viewport, CoordinateTripodOverlay

vp = Viewport(type = Viewport.Type.Ortho)
tripod = CoordinateTripodOverlay(size = 0.07)
vp.overlays.append(tripod)