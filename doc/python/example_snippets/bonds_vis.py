from ovito.io import import_file
from ovito.modifiers import CreateBondsModifier
from ovito.vis import BondsVis

# >>>>>>>>
pipeline = import_file('input/bonds.data.gz', atom_style='bond')
bonds_vis = pipeline.source.bonds['Topology'].vis
bonds_vis.width = 0.4
# <<<<<<<<

# >>>>>>>>
modifier = CreateBondsModifier(cutoff = 2.8)
modifier.vis.shading = BondsVis.Shading.Flat
pipeline.modifiers.append(modifier)
# <<<<<<<<
