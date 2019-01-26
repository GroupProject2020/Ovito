from ovito.io import import_file
from ovito.modifiers import SliceModifier

pipeline = import_file("../files/CFG/shear.void.120.cfg")

modifier = SliceModifier()
pipeline.modifiers.append(modifier)

print(modifier.operate_on)
print('test' in modifier.operate_on)
print('particles' in modifier.operate_on)
print('surfaces' in modifier.operate_on)
modifier.operate_on.remove('particles')
print(modifier.operate_on)
modifier.operate_on.add('particles')
print(modifier.operate_on)
modifier.operate_on = {'surfaces'}
print(modifier.operate_on)
modifier.operate_on.clear()
print(modifier.operate_on)
modifier.operate_on = {'surfaces', 'particles', 'particles'}
print(modifier.operate_on)

pipeline.add_to_scene()
