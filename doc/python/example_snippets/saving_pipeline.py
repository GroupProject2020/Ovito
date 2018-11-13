import ovito
from ovito.io import import_file
from ovito.modifiers import CoordinationAnalysisModifier

pipeline = import_file("input/simulation.dump")
pipeline.modifiers.append(CoordinationAnalysisModifier(cutoff = 3.4))
# ... 
pipeline.add_to_scene()
ovito.scene.save("output/mypipeline.ovito")
############## End of 1st snippet ##################

############# Begin of 2nd snippet #################
import ovito

pipeline = ovito.scene.pipelines[0]
pipeline.source.load("input/second_file.dump") # Replace pipeline input.
pipeline.modifiers[0].cutoff = 3.1             # Adjust modifier params.
data = pipeline.compute()