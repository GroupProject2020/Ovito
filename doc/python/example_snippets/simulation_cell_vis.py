from ovito.io import import_file

pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()

cell_vis = pipeline.source.data.cell.vis
cell_vis.line_width = 1.3
cell_vis.rendering_color = (0.0, 0.0, 0.8)