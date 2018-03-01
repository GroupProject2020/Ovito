import ovito

o1 = ovito.PipelineSceneNode()
print(str(o1))
print(repr(o1))
assert(o1 == o1)
o2 = ovito.PipelineSceneNode()
assert(o1 != o2)
assert(ovito.dataset.anim == ovito.dataset.anim)