from ovito.io import import_file
pipeline = import_file("simulation.dump")

from ovito.modifiers import ComputePropertyModifier

pipeline.modifiers.append(ComputePropertyModifier(
    output_property = 'Color',
    expressions = ['Position.X / CellSize.X', '0.0', '0.5']
))
