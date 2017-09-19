from ovito.io import import_file
from ovito.data import Bonds, BondsEnumerator

# Load a system of atoms and bonds.
pipeline = import_file('bonds.data.gz', atom_style = 'bond')
bonds = pipeline.source.expect(Bonds)
positions = pipeline.source.particle_properties['Position']

# Create bond enumerator object.
bonds_enum = BondsEnumerator(bonds)

# Loop over atoms.
for particle_index in range(len(positions)):
    # Loop over bonds of current atom.
    for bond_index in bonds_enum.bonds_of_particle(particle_index):
        atomA = bonds[bond_index, 0]
        atomB = bonds[bond_index, 1]
        assert(atomA == particle_index or atomB == particle_index)
        print("Atom %i has a bond to atom %i" % (atomA, atomB))
