from ovito.io import import_file
pipeline = import_file("input/simulation.dump")

from ovito.modifiers import ColorCodingModifier

pipeline.modifiers.append(ColorCodingModifier(
    property = 'Potential Energy',
    gradient = ColorCodingModifier.Hot()
))
