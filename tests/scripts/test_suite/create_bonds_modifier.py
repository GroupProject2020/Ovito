from ovito.io import *
from ovito.data import *
from ovito.modifiers import *
import numpy as np

pipeline = import_file("../../files/CFG/shear.void.120.cfg")

modifier = CreateBondsModifier()
pipeline.modifiers.append(modifier)

print("Parameter defaults:")

print("  cutoff: {}".format(modifier.cutoff))
modifier.cutoff = 3.1

print("  intra_molecule_only: {}".format(modifier.intra_molecule_only))
modifier.intra_molecule_only = True

print("  lower_cutoff: {}".format(modifier.lower_cutoff))
modifier.lower_cutoff = 0.1

print("  mode: {}".format(modifier.mode))
modifier.mode = CreateBondsModifier.Mode.Uniform

modifier.bond_type.color = (0.3, 0.4, 0.5)

data = pipeline.compute()

print("Output:")
assert(data.number_of_full_bonds == len(data.particles.bonds['Topology']))
assert(data.particles.bonds.count == 21894/2)
assert(data.particles.bonds.bond_types.types[0] is modifier.bond_type)
assert(np.all(data.particles.bonds.bond_types[...] == 1))

bond_enumerator = BondsEnumerator(data.bonds)
for bond_index in bond_enumerator.bonds_of_particle(0):
    print("Bond index 0:", bond_index)
for bond_index in bond_enumerator.bonds_of_particle(1):
    print("Bond index 1:", bond_index)

# Pair-wise bond creation
modifier.mode = CreateBondsModifier.Mode.Pairwise
modifier.set_pairwise_cutoff(1, 2, 3.0)
data = pipeline.compute()
assert(data.particles.bonds.count == 396)

# Pair-wise bond creation.
pipeline.source.load("../../files/POSCAR/Ti_n1_PBE.n54_G7_V15.000.poscar.000")
modifier.mode = CreateBondsModifier.Mode.Pairwise
modifier.set_pairwise_cutoff("W", "Ti", 3.0)
assert(modifier.get_pairwise_cutoff("Ti", "W") == 3.0)
data = pipeline.compute()
assert(data.particles.bonds.count == 8)
assert(np.all(data.particles.bonds.bond_types[...] == 1))
