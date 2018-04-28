from ovito import scene
from ovito.io import import_file

pipeline1 = import_file('input/first_file.dump')
pipeline2 = import_file('input/second_file.dump')

pipeline1.add_to_scene()
assert(pipeline1 in scene.pipelines)
pipeline1.remove_from_scene()

scene.pipelines.append(pipeline2)
assert(pipeline2 in scene.pipelines)
del scene.pipelines[scene.pipelines.index(pipeline2)]