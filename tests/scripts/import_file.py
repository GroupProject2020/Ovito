import ovito
from ovito.io import import_file
import numpy as np

test_data_dir = "../files/"

node1 = import_file(test_data_dir + "LAMMPS/animation.dump.gz")
assert(len(ovito.scene.pipelines) == 0)
import_file(test_data_dir + "CFG/fcc_coherent_twin.0.cfg")
import_file(test_data_dir + "CFG/shear.void.120.cfg")
import_file(test_data_dir + "LAMMPS/very_small_fp_number.dump")
import_file(test_data_dir + "Parcas/movie.0000000.parcas")
import_file(test_data_dir + "IMD/nw2.imd.gz")
import_file(test_data_dir + "FHI-aims/3_geometry.in.next_step")
import_file(test_data_dir + "GSD/test.gsd")
node = import_file(test_data_dir + "GSD/E.gsd")
assert(node.source.num_frames == 5)
import_file(test_data_dir + "GSD/triblock.gsd")
import_file(test_data_dir + "PDB/SiShuffle.pdb")
import_file(test_data_dir + "PDB/trjconv_gromacs.pdb")
import_file(test_data_dir + "POSCAR/Ti_n1_PBE.n54_G7_V15.000.poscar.000")
import_file(test_data_dir + "NetCDF/C60_impact.nc")
import_file(test_data_dir + "NetCDF/sheared_aSi.nc")
import_file(test_data_dir + "DL_POLY/CONFIG.gz")
import_file(test_data_dir + "LAMMPS/atom_style_sphere.data.gz")
import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
import_file(test_data_dir + "LAMMPS/bonds.data.gz", atom_style = "bond")
import_file(test_data_dir + "LAMMPS/system.hybrid_style.data")
node = import_file(test_data_dir + "XYZ/pyridine.xyz")
assert(len(node.source.data.particles.keys()) == 2)
node = import_file(test_data_dir + "XYZ/pyridine_five.xyz",
    columns = ["Particle Type", "Position.X", "Position.Y", "Position.Z", "Particle Identifier"])
assert(len(node.source.data.particles.keys()) == 3)
node = import_file(test_data_dir + "XYZ/LiH.xyz")
assert(len(node.source.data.particles.keys()) == 3)
node = import_file(test_data_dir + "XYZ/negative_atom_types.xyz")
assert(len(node.source.data.particles.keys()) == 2)
import_file(test_data_dir + "XSF/1.xsf")
import_file(test_data_dir + "XSF/1symb.xsf")
import_file(test_data_dir + "XSF/c2h4_Ag001.xsf")
import_file(test_data_dir + "XSF/GaAsH.xsf")
import_file(test_data_dir + "XSF/vector_field.xsf")
import_file(test_data_dir + "XSF/ZnS.xsf")
node = import_file(test_data_dir + "XSF/ZnS_variable-cell.axsf")
assert(node.source.num_frames == 13)
node = import_file(test_data_dir + "POSCAR/XDATCAR.dms")
assert(node.source.num_frames == 10)
assert(node.compute(2).attributes['SourceFrame'] == 2)
node = import_file(test_data_dir + "POSCAR/XDATCAR_ISIF2.txt")
assert(node.source.num_frames == 9)
node = import_file(test_data_dir + "POSCAR/XDATCAR_ISIF3.txt")
assert(node.source.num_frames == 18)
node = import_file(test_data_dir + "POSCAR/XDATCAR_ISIF4.txt")
assert(node.source.num_frames == 9)
import_file(test_data_dir + "CUBE/test_5.0_equalweights_complete.cube.gz")
import_file(test_data_dir + "CUBE/grid20.cube.gz")
import_file(test_data_dir + "CUBE/grid20ang.cube.gz")
import_file(test_data_dir + "CUBE/grid25mo.cube.gz")
import_file(test_data_dir + "CUBE/valtest.cube.gz")
data = import_file(test_data_dir + "CUBE/grid20mo6-8.cube.gz").compute()
assert(data.grids['imported']['Property'].shape[1] == 3)
import_file(test_data_dir + "GALAMOST/gb.xml")
import_file(test_data_dir + "GALAMOST/particles.0010000000.xml")
import_file(test_data_dir + "QE/Si.in")
import_file(test_data_dir + "QE/QE_Input_File.in")
import_file(test_data_dir + "QE/2.in")
import_file(test_data_dir + "CASTEP/test.cell")
node = import_file(test_data_dir + "CASTEP/quartz_alpha.geom")
assert(node.source.num_frames == 20)
node = import_file(test_data_dir + "CASTEP/test.md")
assert(node.source.num_frames == 5)
import_file(test_data_dir + "VTK/mesh_test.vtk")
import_file(test_data_dir + "VTK/ThomsonTet_Gr1_rotmatNonRand_unstructGrid.vtk")
import_file(test_data_dir + "VTK/box_a.vtk")
import_file(test_data_dir + "OBJ/B.obj")
import_file(test_data_dir + "STL/A-ASCII.stl")
pipeline = import_file(test_data_dir + "LAMMPS/multi_sequence_*.dump")
assert(pipeline.source.num_frames == 11)
pipeline = import_file([test_data_dir + "LAMMPS/multi_sequence_1.dump", test_data_dir + "LAMMPS/multi_sequence_2.dump", test_data_dir + "LAMMPS/multi_sequence_3.dump"])
assert(pipeline.source.num_frames == 11)
pipeline = import_file([test_data_dir + "LAMMPS/multi_sequence_*.dump", test_data_dir + "LAMMPS/animation1.dump"])
assert(pipeline.source.num_frames == 22)
pipeline = import_file([test_data_dir + "LAMMPS/very_small_fp_number.dump", test_data_dir + "LAMMPS/multi_sequence_*.dump"])
assert(pipeline.source.num_frames == 4)
pipeline = import_file([test_data_dir + "LAMMPS/very_small_fp_number.dump", test_data_dir + "LAMMPS/multi_sequence_*.dump"], multiple_frames = True)
assert(pipeline.source.num_frames == 12)
pipeline = import_file(test_data_dir + "LAMMPS/shear.void.dump.bin",
                            columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
ids = pipeline.compute().particles['Particle Identifier']
assert(not np.all(ids[:-1] <= ids[1:]))
pipeline = import_file(test_data_dir + "LAMMPS/shear.void.dump.bin",
                            columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"],
                            sort_particles = True)
ids = pipeline.compute().particles['Particle Identifier']
assert(np.all(ids[:-1] <= ids[1:]))
try:
    # This should generate an error:
    print("Note: The following error message is intentional.")
    pipeline = import_file(test_data_dir + "LAMMPS/shear.void.dump.bin",
                                columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z", "ExtraProperty"])
    assert False
except RuntimeError:
    pass

pipeline = import_file(test_data_dir + "LAMMPS/animation1.dump")
assert(pipeline.source.num_frames == 11)
pipeline = import_file(test_data_dir + "LAMMPS/animation1.dump", multiple_frames = True)
assert(pipeline.source.num_frames == 11)

pipeline = import_file(test_data_dir + "LAMMPS/shear.void.dump.bin",
                            columns = ["Particle Identifier", None, "Position.X", "Position.Y", "Position.Z"])
pipeline.source.load(test_data_dir + "LAMMPS/shear.void.dump.bin",
                            columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])

import_file(test_data_dir + "LAMMPS/binary_dump.x86_64.bin", columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
import_file(test_data_dir + "LAMMPS/binary_dump.bgq.bin", columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
