import sys
from ovito.io import *
from ovito.io.ase import *
from ovito.data import *
from ovito.pipeline import *

try:
    from ase.atoms import Atoms
except ImportError:
    # Cannot perform following tests without the ASE module being installed on the system.
    print("Note: Skipping tests of ASE interface, because the ASE module is not installed.")
    sys.exit()

pipeline = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')
data = pipeline.source.compute()

atoms = ovito_to_ase(data)
print(atoms)

data = ase_to_ovito(atoms)
assert(isinstance(data, DataCollection))

# Backward compatibility with OVITO 2.9.0:
atoms = data.to_ase_atoms()
print(atoms)

# Backward compatibility with OVITO 2.9.0:
data2 = DataCollection.create_from_ase_atoms(atoms)
assert(isinstance(data2, DataCollection))
