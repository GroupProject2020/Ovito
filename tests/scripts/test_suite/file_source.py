from ovito.io import *
from ovito.modifiers import *
from ovito.data import *
import numpy as np

node = import_file("../../files/CFG/shear.void.120.cfg")
file_source = node.source

print("source_path:", file_source.source_path)
print("loaded_file:", file_source.loaded_file)
print("num_frames:", file_source.num_frames)
print(file_source)
assert(isinstance(file_source, DataCollection))
assert(file_source.loaded_frame == 0)
assert(file_source.num_frames == 1)
assert(file_source.adjust_animation_interval == True)
assert(file_source.loaded_file.endswith("/shear.void.120.cfg"))