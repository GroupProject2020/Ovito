from ovito.modifiers import HistogramModifier
from ovito.io import import_file, export_file

pipeline = import_file("input/simulation.dump")

modifier = HistogramModifier(bin_count=100, property='peatom')
pipeline.modifiers.append(modifier)

export_file(pipeline, "output/histogram.txt", "txt/series", key="histogram[peatom]")