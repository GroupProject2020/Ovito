from ovito.io import *

node = import_file("../../files/CFG/shear.void.120.cfg")

print(node.source)
print("Number of data objects: ", len(node.source.objects))

print(node.source.particle_properties)

print(list(node.source.particle_properties.keys()))
print(list(node.source.particle_properties.values()))

print(node.source.particle_properties["Position"])

pos = node.source.particle_properties.position
print("pos=", pos)
print("pos.array=", pos.array)
print("pos.array[0]=", pos.array[0])
print("pos[0]=", pos[0])
print("pos[0:3]=", pos[0:3])
print("len(pos)={}".format(len(pos)))
print("pos.marray=", pos.marray)

with pos.modify() as a:
    a[0][0] = 1.0

assert(pos[0][0] == 1.0)