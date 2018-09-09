from ovito.io import *
from ovito.data import *
from ovito.modifiers import *
import numpy

def verify_operation(input_file1, input_file2):

    print("Combining file '{}' with file '{}':".format(input_file1, input_file2))

    pipeline = import_file(input_file1)
    modifier = CombineDatasetsModifier()
    modifier.source.load(input_file2)
    pipeline.modifiers.append(modifier)

    data = pipeline.compute()
    source_data1 = pipeline.source.compute()
    source_data2 = modifier.source.compute()

    # Make sure the particle properties of both input datasets are in the output:
    print("Particle properties in input dataset 1:")
    input_prop_names_1 = set(source_data1.particles.keys())
    print(input_prop_names_1)
    print("Particle properties in input dataset 2:")
    input_prop_names_2 = set(source_data2.particles.keys())
    print(input_prop_names_2)
    print("Particle properties in output dataset:")
    output_prop_names = set(data.particles.keys())
    print(output_prop_names)
    assert(output_prop_names == input_prop_names_1 | input_prop_names_2)

    # Make sure all particles from both input datasets are in the output:
    N_input1 = len(source_data1.particles['Position'])
    N_input2 = len(source_data2.particles['Position'])
    N_output = len(data.particles['Position'])
    print("Number of input particles in dataset 1: ", N_input1)
    print("Number of input particles in dataset 2: ", N_input2)
    print("Number of output particles: ", N_output)
    assert(N_output == N_input1 + N_input2)

    # Check if the particle types have been combined correctly.
    input_types_1 = source_data1.particles['Particle Type'].types
    input_types_2 = source_data2.particles['Particle Type'].types
    output_types = data.particles['Particle Type'].types
    print("Particle types from input dataset 1:")
    for t in input_types_1: print("  {} = {}".format(t.id, t.name))
    print("Particle types from input dataset 2:")
    for t in input_types_2: print("  {} = {}".format(t.id, t.name))
    print("Particle types from output dataset:")
    for t in output_types: print("  {} = {}".format(t.id, t.name))
    assert(set([t.name for t in output_types]) == set([t.name for t in input_types_1]) | set([t.name for t in input_types_2]))

    # Check property contents:
    for name in input_prop_names_1:
        print("Checking values of property '{}'".format(name))
        prop_in = source_data1.particles[name]
        prop_out = data.particles[name]
        assert(numpy.all(prop_out[0:N_input1] == prop_in[...]))
    for name in input_prop_names_2:
        if name == 'Particle Identifier': continue
        if name == 'Molecule Identifier': continue
        print("Checking values of property '{}'".format(name))
        prop_in = source_data2.particles[name]
        prop_out = data.particles[name]
        if not prop_out.types:
            assert(numpy.all(prop_out[N_input1:] == prop_in[...]))
        else:
            for i in range(N_input2):
                assert(prop_out.type_by_id(prop_out[i+N_input1]).name == prop_in.type_by_id(prop_in[i]).name)

    if source_data1.bonds or source_data2.bonds:
        # Make sure the bond properties of both input datasets are in the output:
        print("Bond properties in input dataset 1:")
        input_prop_names_1 = set(source_data1.bonds.keys())
        print(input_prop_names_1)
        print("Bond properties in input dataset 2:")
        input_prop_names_2 = set(source_data2.bonds.keys())
        print(input_prop_names_2)
        print("Bond properties in output dataset:")
        output_prop_names = set(data.bonds.keys())
        print(output_prop_names)
        assert(output_prop_names == input_prop_names_1 | input_prop_names_2)

        # Make sure all bond from both input datasets are in the output:
        if source_data1.bonds.count != 0 and source_data2.bonds.count != 0:
            N_bonds_input1 = source_data1.bonds.count
            N_bonds_input2 = source_data2.bonds.count
            N_bonds_output = data.bonds.count
            print("Number of input bonds in dataset 1: ", N_bonds_input1)
            print("Number of input bonds in dataset 2: ", N_bonds_input2)
            print("Number of output bonds: ", N_bonds_output)
            assert(N_bonds_output == N_bonds_input1 + N_bonds_input2)

            # Check if the bnd types have been combined correctly.
            input_types_1 = source_data1.bonds['Bond Type'].types
            input_types_2 = source_data2.bonds['Bond Type'].types
            output_types = data.bonds['Bond Type'].types
            print("Bond types from input dataset 1:")
            for t in input_types_1: print("  {} = {}".format(t.id, t.name))
            print("Bond types from input dataset 2:")
            for t in input_types_2: print("  {} = {}".format(t.id, t.name))
            print("Bond types from output dataset:")
            for t in output_types: print("  {} = {}".format(t.id, t.name))
            assert(set([t.name for t in output_types]) == set([t.name for t in input_types_1]) | set([t.name for t in input_types_2]))

            # Check property contents:
            for name in input_prop_names_1:
                print("Checking values of property '{}'".format(name))
                prop_in = source_data1.bonds[name]
                prop_out = data.bonds[name]
                assert(numpy.all(prop_out[0:N_bonds_input1] == prop_in[...]))
            for name in input_prop_names_2:
                print("Checking values of property '{}'".format(name))
                prop_in = source_data2.bonds[name]
                prop_out = data.bonds[name]
                if name == 'Topology':
                    assert(numpy.all(prop_out[N_bonds_input1:] == prop_in[...] + N_input1))
                elif not prop_out.types:
                    assert(numpy.all(prop_out[N_bonds_input1:] == prop_in[...]))
                else:
                    for i in range(N_bonds_input2):
                        assert(prop_out.type_by_id(prop_out[i+N_bonds_input1]).name == prop_in.type_by_id(prop_in[i]).name)
        

verify_operation("../../files/XYZ/SiH.extended.xyz", "../../files/XYZ/LiH.xyz")
verify_operation("../../files/LAMMPS/class2.data", "../../files/LAMMPS/water.start.data.gz")
