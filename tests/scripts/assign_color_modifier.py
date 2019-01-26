from ovito.io import *
from ovito.modifiers import *

import numpy

pipeline = import_file("../files/LAMMPS/animation.dump.gz")
pipeline.modifiers.append(AssignColorModifier(color = (0,1,0)))

assert((pipeline.compute().particles.colors == numpy.array([0,1,0])).all())
