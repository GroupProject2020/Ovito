# Skip execution of this example script during testing if ASE is not installed.
import sys
try: import ase
except: sys.exit()
# >>>>>>>>>>>>>>>>>>>>>>> begin example snippet >>>>>>>>>>>>>>>>>>
from ovito.pipeline import StaticSource, Pipeline
from ovito.io.ase import ase_to_ovito
from ase.atoms import Atoms

# The ASE Atoms object to convert:
ase_atoms = Atoms('CO', positions=[(0, 0, 0), (0, 0, 1.1)])

# Create a new Pipeline with a StaticSource as data source:
pipeline = Pipeline(source = StaticSource())

# Convert the ASE object; use the StaticSource as destination container:
ase_to_ovito(ase_atoms, pipeline.source)
