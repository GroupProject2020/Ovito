import ovito
from ovito.pipeline import Pipeline

print(ovito.dataset)
assert(isinstance(ovito.dataset, ovito.DataSet))
assert(ovito.dataset == ovito.dataset.anim.dataset)

scene_nodes = ovito.dataset.scene_nodes
assert(len(scene_nodes) == 0)
assert(ovito.dataset.selected_node is None)

o1 = Pipeline()
o1.add_to_scene()
assert(len(scene_nodes) == 1)
assert(scene_nodes[0] == o1)
assert(o1 in scene_nodes)
assert(ovito.dataset.selected_node == o1)

o2 = Pipeline()
scene_nodes.append(o2)
assert(len(scene_nodes) == 2)
assert(scene_nodes[1] == o2)
ovito.dataset.selected_node = o2

o1.remove_from_scene()
assert(len(scene_nodes) == 1)
assert(scene_nodes[0] == o2)

del scene_nodes[0]
assert(len(scene_nodes) == 0)
