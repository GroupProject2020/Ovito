from ovito.io import import_file

pipeline = import_file("../files/CFG/shear.void.120.cfg")
data = pipeline.compute()

# For backward compatibility with OVITO 2.9.0:
print(data.particle_properties)
print(data.particle_properties.position)
print(data.particles.position)

# Test property list access.
print(data.particles)
print(list(data.particles.keys()))
print(list(data.particles.values()))

# Test property access:
pos = data.particles['Position']
print("pos=", pos)
print("pos[0]=", pos[0])
print("pos[0:3]=", pos[0:3])
print("len(pos)={}".format(len(pos)))
assert(len(pos) == data.particles.count)
assert(data.particles.positions is pos)

# Test property write access:
with data.particles_.positions_:
    data.particles_.positions[3] = (1,2,3)
mutable_pos = data.particles_['Position_']
with mutable_pos:
    mutable_pos[0][0] = 1.234
assert(mutable_pos[0,0] == 1.234)
assert(mutable_pos is not pos)

# Backward compatibility with OVITO 2.9.0:
print("pos.array=", pos.array)
print("pos.array[0]=", pos.array[0])
print("pos.marray=", mutable_pos.marray)

