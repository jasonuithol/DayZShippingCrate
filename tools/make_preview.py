#!/usr/bin/env python3
"""Draws the Workshop picture, ShippingCrate/preview.png (1024x1024, under 1 MB): a shipping
container coming down under two parachutes over Chernarus-ish hills, and far up in the sky the
little plane that dropped it.

    python3 tools/make_preview.py
"""
import math, os, random
from PIL import Image, ImageDraw, ImageFilter, ImageFont

S = 2048                      # drawn at twice the size, scaled down for smooth edges
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'ShippingCrate', 'preview.png')
FONTS = '/usr/share/fonts'
rng = random.Random(2026)

def font(name, size):
    for root, _, files in os.walk(FONTS):
        if name in files:
            return ImageFont.truetype(os.path.join(root, name), size)
    raise SystemExit('font not found: ' + name)

def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))

img = Image.new('RGB', (S, S))
d = ImageDraw.Draw(img)

# sky: deep blue at the top to a warm morning haze at the horizon
TOP, MID, HAZE = (32, 62, 104), (86, 132, 170), (232, 196, 150)
for y in range(S):
    t = y / S
    c = lerp(TOP, MID, t / 0.55) if t < 0.55 else lerp(MID, HAZE, min(1, (t - 0.55) / 0.3))
    d.line([(0, y), (S, y)], fill=c)

# a few soft clouds
clouds = Image.new('RGBA', (S, S), (0, 0, 0, 0))
cd = ImageDraw.Draw(clouds)
for cx, cy, w in [(0.18, 0.42, 0.30), (0.80, 0.30, 0.26), (0.62, 0.55, 0.34), (0.08, 0.62, 0.22)]:
    for _ in range(14):
        x = (cx + rng.uniform(-w / 2, w / 2)) * S
        y = (cy + rng.uniform(-0.02, 0.02)) * S
        r = rng.uniform(0.03, 0.07) * S
        cd.ellipse([x - r * 1.8, y - r, x + r * 1.8, y + r], fill=(255, 244, 232, 70))
img.paste(clouds.filter(ImageFilter.GaussianBlur(S * 0.012)), (0, 0), clouds.filter(ImageFilter.GaussianBlur(S * 0.012)))
d = ImageDraw.Draw(img)

# the plane, tiny and high up on the left, with its contrail running back off the edge
px, py = int(S * 0.16), int(S * 0.27)
trail = Image.new('RGBA', (S, S), (0, 0, 0, 0))
td = ImageDraw.Draw(trail)
td.line([(px + 24, py + 2), (S * 1.05, py - S * 0.035)], fill=(255, 255, 255, 120), width=10)
img.paste(trail.filter(ImageFilter.GaussianBlur(6)), (0, 0), trail.filter(ImageFilter.GaussianBlur(6)))
d = ImageDraw.Draw(img)
PLANE = (38, 44, 52)
k = 1.6                                                     # plane scale
d.polygon([(px - 40 * k, py + 4 * k), (px + 30 * k, py - 3 * k), (px + 38 * k, py + 1 * k), (px - 36 * k, py + 9 * k)], fill=PLANE)   # fuselage
d.polygon([(px - 6 * k, py + 2 * k), (px + 8 * k, py - 26 * k), (px + 14 * k, py - 25 * k), (px + 6 * k, py + 2 * k)], fill=PLANE)   # far wing
d.polygon([(px - 8 * k, py + 5 * k), (px + 2 * k, py + 28 * k), (px + 9 * k, py + 28 * k), (px + 4 * k, py + 5 * k)], fill=PLANE)    # near wing
d.polygon([(px + 26 * k, py - 2 * k), (px + 36 * k, py - 16 * k), (px + 40 * k, py - 15 * k), (px + 36 * k, py + 0 * k)], fill=PLANE) # tail fin

# hills: three layers, paler with distance
def hills(base, amp, colour, seed):
    r = random.Random(seed)
    phases = [r.uniform(0, 6.28) for _ in range(4)]
    pts = [(0, S)]
    for x in range(0, S + 16, 16):
        y = base
        for i, p in enumerate(phases):
            y += amp / (i + 1) * math.sin(x / S * (2 + i * 2.3) * math.pi + p)
        pts.append((x, y))
    pts.append((S, S))
    d.polygon(pts, fill=colour)

hills(S * 0.80, S * 0.030, (126, 140, 132), 1)
hills(S * 0.85, S * 0.035, (78, 98, 76), 2)
hills(S * 0.91, S * 0.025, (44, 62, 42), 3)
# a dark tree line on the nearest hill
for x in range(0, S, 14):
    h = rng.uniform(0.012, 0.03) * S
    base = S * 0.905 + 0.025 * S * math.sin(x / S * 2 * math.pi + 1.3)
    d.polygon([(x - 10, base), (x, base - h), (x + 10, base)], fill=(30, 44, 30))

# the container: a 20 ft box seen a little from the side, door end towards us on the right
cx, cy = S * 0.50, S * 0.585          # centre of the long face
L, H, DEPTH = S * 0.40, S * 0.15, S * 0.10
ORANGE, ORANGE_DARK, ORANGE_TOP = (214, 108, 38), (160, 74, 26), (238, 146, 74)
x0, x1 = cx - L / 2, cx + L / 2
y0, y1 = cy - H / 2, cy + H / 2
dx, dy = DEPTH * 0.75, -DEPTH * 0.45   # receding direction (end face and roof)
long_face = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
end_face = [(x1, y0), (x1 + dx, y0 + dy), (x1 + dx, y1 + dy), (x1, y1)]
roof = [(x0, y0), (x0 + dx, y0 + dy), (x1 + dx, y0 + dy), (x1, y0)]

# lines from both canopies, drawn first so the box covers their ends
canopies = [(cx - L * 0.30, S * 0.335), (cx + L * 0.36, S * 0.315)]
corners = [[(x0 + 6, y0), (x0 + dx + 6, y0 + dy), (cx - 20, y0 + dy * 0.5)],
           [(x1 - 6, y0), (x1 + dx - 6, y0 + dy), (cx + 20, y0 + dy * 0.5)]]
CANOPY_W = S * 0.26
for (ax, ay), ends in zip(canopies, corners):
    for i in range(7):
        sx = ax - CANOPY_W / 2 + CANOPY_W * (0.06 + 0.88 * i / 6)
        sy = ay + S * 0.018
        ex, ey = ends[i % len(ends)]
        d.line([(sx, sy), (ex, ey)], fill=(236, 230, 214), width=4)

d.polygon(roof, fill=ORANGE_TOP)
d.polygon(long_face, fill=ORANGE)
d.polygon(end_face, fill=ORANGE_DARK)
# corrugation on the long face and the roof
for i in range(1, 30):
    x = x0 + L * i / 30
    d.line([(x, y0 + 6), (x, y1 - 6)], fill=(186, 88, 30), width=6)
    d.line([(x, y0), (x + dx, y0 + dy)], fill=(222, 128, 60), width=3)
# doors on the end face: two leaves, locking bars
mid_top = (x1 + dx / 2, y0 + dy / 2)
d.line([mid_top, (x1 + dx / 2, y1 + dy / 2)], fill=(110, 50, 18), width=6)
for f in (0.22, 0.40, 0.60, 0.78):
    bx = x1 + dx * f
    d.line([(bx, y0 + dy * f + 14), (bx, y1 + dy * f - 14)], fill=(196, 188, 176), width=5)
# frame: dark corner posts and rails
FRAME = (74, 40, 18)
for poly in (long_face, end_face, roof):
    d.line(poly + [poly[0]], fill=FRAME, width=8, joint='curve')
# a stencilled marking on the side
d.text((x0 + L * 0.06, y0 + H * 0.18), 'DZSC', font=font('DejaVuSans-Bold.ttf', int(S * 0.028)), fill=(250, 226, 200))

# the canopies: domes with alternating panels and a scalloped skirt
for (ax, ay) in canopies:
    w, h = CANOPY_W, S * 0.10
    dome = Image.new('RGBA', (int(w) + 20, int(h * 1.4)), (0, 0, 0, 0))
    dd = ImageDraw.Draw(dome)
    ox, oy = 10, 10
    dd.chord([ox, oy, ox + w, oy + h * 2], 180, 360, fill=(238, 232, 214))
    panels = 8
    for p in range(panels):
        if p % 2:
            a0 = 180 + 180 * p / panels
            a1 = 180 + 180 * (p + 1) / panels
            dd.pieslice([ox, oy, ox + w, oy + h * 2], a0, a1, fill=(110, 122, 74))
    # skirt scallops
    for p in range(panels):
        sx = ox + w * p / panels
        dd.chord([sx, oy + h - h * 0.10, sx + w / panels, oy + h + h * 0.14], 0, 180, fill=(238, 232, 214) if p % 2 == 0 else (110, 122, 74))
    dd.arc([ox, oy, ox + w, oy + h * 2], 180, 360, fill=(70, 74, 60), width=5)
    img.paste(dome, (int(ax - w / 2 - ox), int(ay - h - oy + S * 0.02)), dome)
d = ImageDraw.Draw(img)

# title
title = font('DejaVuSans-Bold.ttf', int(S * 0.115))
w = d.textlength('SHIPPING CRATE', font=title)
if w > S * 0.90:                                            # fit the width, with a margin
    title = font('DejaVuSans-Bold.ttf', int(S * 0.115 * S * 0.90 / w))
def centred(text, f, y, fill, shadow=(20, 28, 40)):
    w = d.textlength(text, font=f)
    x = (S - w) / 2
    d.text((x + 8, y + 8), text, font=f, fill=shadow)
    d.text((x, y), text, font=f, fill=fill)
centred('SHIPPING CRATE', title, S * 0.035, (250, 238, 220))
centred('Expansion airdrops, by shipping container', font('DejaVuSans-Bold.ttf', int(S * 0.034)), S * 0.165, (232, 214, 190))

out = img.resize((1024, 1024), Image.LANCZOS)
out.save(OUT, optimize=True)
print('wrote', os.path.normpath(OUT), os.path.getsize(OUT), 'bytes')
