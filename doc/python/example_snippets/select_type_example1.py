from ovito.io import import_file
from ovito.modifiers import SelectTypeModifier

pipeline = import_file("../../../tests/files/CFG/fcc_coherent_twin.0.cfg")

# >>>>>>>>>>> begin snippet >>>>>>>>>>>>>>
modifier = SelectTypeModifier(property = 'Particle Type', types = {'Cu'})
# >>>>>>>>>>>> end snippet >>>>>>>>>>>>>>>

pipeline.modifiers.append(modifier)
data = pipeline.compute()
assert(data.attributes['SelectType.num_selected'] > 0)