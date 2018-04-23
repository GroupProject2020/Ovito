from ovito.pipeline import StaticSource, Pipeline
from ovito.data import DataCollection, SimulationCell
from ovito.modifiers import CreateBondsModifier
from ovito.io import export_file

# Insert a new SimulationCell object into a data collection:
data = DataCollection()
cell = SimulationCell(pbc = (False, False, False))
with cell:
    cell[:,0] = (4,0,0)
    cell[:,1] = (0,2,0)
    cell[:,2] = (0,0,2)
data.objects.append(cell)

# Create two particles:
xyz = [[0,0,0],[2,0,0]]
data.particles.create_property('Position', data = xyz)

# Create a new Pipeline with a StaticSource as data source:
pipeline = Pipeline(source = StaticSource())
pipeline.source.assign(data)

# Apply a modifier:
pipeline.modifiers.append(CreateBondsModifier(cutoff = 3.0))

# Write pipeline results to an output file:
export_file(pipeline, 'output/structure.data', 'lammps/data', atom_style='bond')