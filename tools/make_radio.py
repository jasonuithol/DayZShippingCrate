#!/usr/bin/env python3
"""Make ShippingCrate/data/sc_radio.p3d, the delivery radio: a copy of vanilla's binarized
WalkieTalkie.p3d whose colour texture is a solid bright orange. The model has no hidden
selections, so config can't retexture it; instead the texture name stored in each LOD is
replaced by a procedural colour of exactly the same length (the names are null-terminated
strings in the ODOL, so nothing else moves). Materials (shading, damage) stay vanilla's.

    tools/make_radio.py <WalkieTalkie.p3d from gear_radio.pbo> ShippingCrate/data/sc_radio.p3d
"""
import sys

src, dst = sys.argv[1:3]
data = open(src, "rb").read()
old = b"dz\\gear\\radio\\data\\walkie_talkie_co.paa"
new = b"#(argb,8,8,3)color(1.0,0.420,0.02,1,co)"
assert len(new) == len(old)
assert data[:4] == b"ODOL", "not a binarized model"
n = data.count(old)
assert n > 0, "texture name not found"
open(dst, "wb").write(data.replace(old, new))
print(f"{dst}: {n} LODs retextured")
