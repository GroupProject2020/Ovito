from ovito.io import *

pipeline = import_file("../../files/CFG/shear.void.120.cfg")
data = pipeline.compute()

print(data)
print("Number of data objects: ", len(data.objects))

print(data.particle_properties)
print(data.particles)

print(list(data.particles.keys()))
print(list(data.particles.values()))

print(data.particles["Position"])
print(data.particles.position)

pos = data.particles['Position']
print("pos=", pos)
print("pos.array=", pos.array)
print("pos.array[0]=", pos.array[0])
print("pos[0]=", pos[0])
print("pos[0:3]=", pos[0:3])
print("len(pos)={}".format(len(pos)))
print("pos.marray=", pos.marray)
assert(len(pos) == data.particles.count)

with pos:
    pos[0][0] = 1.0

assert(pos[0][0] == 1.0)