from ovito.modifiers import CalculateDisplacementsModifier
from ovito.pipeline import FileSource

# The modifier that requires a reference config:
mod = CalculateDisplacementsModifier()

# Load the reference config from a separate input input file.
mod.reference = FileSource()
mod.reference.load('simulation.0.dump')
