from ovito.io import import_file
from ovito.vis import VectorVis
from ovito.modifiers import CalculateDisplacementsModifier

# >>>>>>>>
pipeline = import_file('input/simulation.dump')
pipeline.add_to_cene()
vector_vis = pipeline.source.particles['Force'].vis
vector_vis.color = (1,0,0)
# <<<<<<<<


# >>>>>>>>
modifier = CalculateDisplacementsModifier()
pipeline.modifiers.append(modifier)
modifier.vis.enabled = True
modifier.vis.shading = VectorVis.Shading.Flat
# <<<<<<<<
