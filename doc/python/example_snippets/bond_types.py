from ovito.io import import_file

# Load a dataset containing atoms and bonds.
pipeline = import_file('input/bonds.data.gz', atom_style='bond')
data = pipeline.compute()

################## Snippet begin ########################
bond_type_list = data.particles.bonds.bond_types.types
for bond_type in bond_type_list:
    print(bond_type.id, bond_type.name, bond_type.color)
################## Snippet end ########################

assert(len(bond_type_list) != 0)