from ovito.io import import_file
from ovito.data import BondsEnumerator
from ovito.modifiers import ComputePropertyModifier

# Load a dataset containing atoms and bonds.
pipeline = import_file('input/bonds.data.gz', atom_style='bond')

# For demonstration purposes, let a modifier calculate the length of each bond.
pipeline.modifiers.append(ComputePropertyModifier(operate_on='bonds', output_property='Length', expressions=['BondLength']))

# Obtain pipeline results.
data = pipeline.compute()
positions = data.particles['Position']  # array with atomic positions
bond_topology = data.bonds['Topology']  # array with bond topology
bond_lengths = data.bonds['Length']     # array with bond lengths

# Create bonds enumerator object.
bonds_enum = BondsEnumerator(data)

# Loop over atoms.
for particle_index in range(data.particles.count):
    # Loop over bonds of current atom.
    for bond_index in bonds_enum.bonds_of_particle(particle_index):
        # Obtain the indices of the two particles connected by the bond:
        a = bond_topology[bond_index, 0]
        b = bond_topology[bond_index, 1]
        
        # Bond orientations can be arbitrary (a->b or b->a):
        assert(a == particle_index or b == particle_index)
        
        # Obtain the length of the bond from the 'Length' bond property:
        length = bond_lengths[bond_index]

        print("Bond from atom %i to atom %i has length %f" % (a, b, length))
