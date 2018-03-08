import ovito
from ovito.pipeline import Pipeline

o1 = Pipeline()
print(str(o1))
print(repr(o1))
assert(o1 == o1)
o2 = Pipeline()
assert(o1 != o2)
assert(ovito.dataset.anim == ovito.dataset.anim)