from ovito.io import import_file
from ovito.vis import TextLabelOverlay, Viewport
from ovito.modifiers import ExpressionSelectionModifier

# Import a simulation dataset and select some atoms based on their potential energy:
pipeline = import_file("input/simulation.dump")
pipeline.add_to_scene()
pipeline.modifiers.append(ExpressionSelectionModifier(expression='peatom > -4.2'))

# Create the overlay. Note that the text string contains a reference
# to an output attribute of the ExpressionSelectionModifier.
overlay = TextLabelOverlay(text = 'Number of selected atoms: [SelectExpression.num_selected]')
# Specify the source of dynamically computed attributes.
overlay.source_pipeline = pipeline

# Attach overlay to a newly created viewport:
viewport = Viewport(type = Viewport.Type.Top)
viewport.overlays.append(overlay)