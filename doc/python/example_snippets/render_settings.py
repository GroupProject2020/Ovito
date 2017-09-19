from ovito.vis import RenderSettings, Viewport, TachyonRenderer

settings = RenderSettings(
    size = (320,240),
    filename = "myfigure.png",
    background_color = (0,0,0),
    renderer = TachyonRenderer(shadows=False, direct_light=False)
)

vp = Viewport(type=Viewport.Type.PERSPECTIVE, camera_dir=(-100, -50, -50))
vp.zoom_all()
vp.render(settings)

import ovito

settings = RenderSettings(
    size = (1280,720), 
    filename = 'movie.mp4', 
    range = RenderSettings.Range.ANIMATION)

ovito.dataset.anim.frames_per_second = 30

settings = RenderSettings(
    size = (1280,720), 
    filename = 'frame.png', 
    range = RenderSettings.Range.ANIMATION)
