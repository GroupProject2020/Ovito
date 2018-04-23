# Skip execution of this example script during testing if ASE is not installed.
import sys
try: import ase
except: sys.exit()
# >>>>>>>>>>>>>>>>>>>>>>> begin example snippet >>>>>>>>>>>>>>>>>>
from ovito.io import import_file
from ovito.io.ase import ovito_to_ase

# Create an OVITO data pipeline from an external file:
pipeline = import_file('input/simulation.dump')

# Evaluate pipeline to obtain a DataCollection:
data = pipeline.compute()

# Convert it to an ASE Atoms object:
ase_atoms = ovito_to_ase(data)
