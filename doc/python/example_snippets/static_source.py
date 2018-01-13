from ovito.pipeline import StaticSource, Pipeline
from ovito.data import SimulationCell
from ovito.modifiers import CreateBondsModifier
from ovito.io import export_file

# Create a new Pipeline with a StaticSource as data source:
pipeline = Pipeline(source = StaticSource())

# Insert a new SimulationCell object into the StaticSource:
cell = SimulationCell(pbc = (False, False, False))
with cell:
    cell[:,0] = (4,0,0)
    cell[:,1] = (0,2,0)
    cell[:,2] = (0,0,2)
pipeline.source.objects.append(cell)

# Insert two particles into the StaticSource:
xyz = [[0,0,0],[2,0,0]]
pipeline.source.particle_properties.create('Position', data=xyz)

# Apply a modifier:
pipeline.modifiers.append(CreateBondsModifier(cutoff = 3.0))

# Write pipeline results to an output file:
export_file(pipeline, 'output/structure.data', 'lammps/data', atom_style='bond')