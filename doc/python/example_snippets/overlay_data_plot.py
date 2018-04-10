test_flag = False

# >>>>>>>>> snippet start here >>>>>>>>>>>>>>>
import matplotlib
matplotlib.use('Agg') # Activate 'agg' backend for off-screen plotting.
import matplotlib.pyplot as plt
import PyQt5.QtGui
import ovito
from ovito.modifiers import HistogramModifier

def render(args):
    # Look up the HistogramModifier in the pipeline whose data we will plot.
    hist_modifier = None
    for mod in ovito.dataset.selected_pipeline.modifiers:
        if isinstance(mod, HistogramModifier):
            hist_modifier = mod
            break
    if not hist_modifier: raise RuntimeError('Histogram modifier not present')

    #  Compute plot size in inches (DPI determines label size)
    dpi = 80
    plot_width = 0.5 * args.size[0] / dpi
    plot_height = 0.5 * args.size[1] / dpi
    
    # Create matplotlib figure:
    fig, ax = plt.subplots(figsize=(plot_width,plot_height), dpi=dpi)
    fig.patch.set_alpha(0.5)
    plt.title('Coordination')
    
    # Plot histogram data
    ax.bar(hist_modifier.histogram[:,0], hist_modifier.histogram[:,1])
    plt.tight_layout()
    
    # Render figure to an in-memory buffer.
    buf = fig.canvas.print_to_buffer()
    
    # Create a QImage from the memory buffer
    res_x, res_y = buf[1]
    img = PyQt5.QtGui.QImage(buf[0], res_x, res_y, PyQt5.QtGui.QImage.Format_RGBA8888)
    
    # Paint QImage onto viewport canvas 
    args.painter.drawImage(0, 0, img)	

# <<<<<<<<<<<<< snippet ends here <<<<<<<<<<<<<
    global test_flag
    test_flag = True # Indicate to the driver script that this function has been executed

from ovito.vis import PythonViewportOverlay, Viewport, TachyonRenderer
from ovito.io import import_file

pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()
pipeline.modifiers.append(HistogramModifier(bin_count=10, property='peatom'))
viewport = Viewport(type = Viewport.Type.Top)
viewport.overlays.append(PythonViewportOverlay(function = render))
viewport.render_image(filename='output/simulation.png', 
                size=(200, 200), 
                renderer=TachyonRenderer())
assert(test_flag)