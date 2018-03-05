from ovito.vis import Viewport, TachyonRenderer
vp = Viewport()

# >>>>>>>>>>>>>>>
vp.render_image(filename='output/simulation.png', 
                size=(320,240),
                background=(0,0,0), 
                renderer=TachyonRenderer(ambient_occlusion=False, shadows=False))