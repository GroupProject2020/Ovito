from ovito.pipeline import StaticSource, Pipeline
from ovito.data import DataCollection, SimulationCell, Particles
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

# Create a Particles object containing two particles:
particles = Particles()
particles.create_property('Position', data=[[0,0,0],[2,0,0]])
data.objects.append(particles)

# Create a new Pipeline with a StaticSource as data source:
pipeline = Pipeline(source = StaticSource(data = data))

# Apply a modifier:
pipeline.modifiers.append(CreateBondsModifier(cutoff = 3.0))

# Write pipeline results to an output file:
export_file(pipeline, 'output/structure.data', 'lammps/data', atom_style='bond')