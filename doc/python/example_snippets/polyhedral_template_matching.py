from ovito.modifiers import PolyhedralTemplateMatchingModifier


# >>>>>>>>>>>>>>>>> Snippet starts here >>>>>>>>>>>>>>>>>>>>>>>
modifier = PolyhedralTemplateMatchingModifier()

# Enable the identification of cubic and hexagonal diamond structures:
modifier.structures[PolyhedralTemplateMatchingModifier.Type.CUBIC_DIAMOND].enabled = True
modifier.structures[PolyhedralTemplateMatchingModifier.Type.HEX_DIAMOND].enabled = True

# Give all cubic diamond atoms a blue color and hexagonal atoms a red color:
modifier.structures[PolyhedralTemplateMatchingModifier.Type.CUBIC_DIAMOND].color = (0.0, 0.0, 1.0)
modifier.structures[PolyhedralTemplateMatchingModifier.Type.HEX_DIAMOND].color = (1.0, 0.0, 0.0)