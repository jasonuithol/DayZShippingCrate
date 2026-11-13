# Changelog

## 0.3.1 - 2026-11-13

- The delivery radio is bright orange, as it was meant to be (it came out black). Vanilla's
  walkie-talkie model can't be retextured from config, so the radio is a copy of it with its
  colour texture made solid orange (`tools/make_radio.py`); its damaged looks are vanilla's.

## 0.3 - 2026-11-13

- Every vanilla car, truck and motorbike: Ada 4x4, Olga 24, Gunter 2, Sarka 120, M1025,
  M3S Covered, Jana 50 and Bitrak 682, each with all its parts and full fluids, in a random
  colour.
- Vehicles too big for the container (the M3S and the M1025) come down on a deck of wooden
  pallets instead, the real vehicle riding on it, with room at the free end for the hand cart,
  a sea chest and a barrel. A load's `carrier` can be `"container"`, `"pallet"` or left out to
  choose by size.
- The M3S's sea chest is full of explosives: C4, IEDs, claymores, landmines, frag, flash, gas
  and smoke grenades, and two remote detonator kits.
- Gun kits (`guns`): VSD, LAR, KA-M, SCR 16, SVAL, R12 and M79. Each kit is four guns, loaded,
  with spare magazines (or ammo), in the vehicle's boot. On a motorbike they lie beside it.
- The delivery radio: a bright orange walkie-talkie with two knobs instead of a frequency, one
  for the vehicle and one for the guns (click for the next vehicle, hold to step through the
  guns; the prompts show what each is on). A drop on a group with a tuned radio brings what
  it's tuned to, read again as the crate leaves the plane. An untuned vehicle knob means a
  random vehicle; an untuned guns knob ("any") fills the boot with one of each gun, round after
  round until it's full. Every new character starts with one.
- A server restart no longer leaves a drop's contents behind: landed drops that haven't blown
  up yet, and vehicles still riding down, are recorded (`profiles/ShippingCrate/landed.json`)
  and cleared away 20 s after the next start.
- The IEDs are stacked upright with the bear sitting on top, so a slope can't bury them. The
  bear's eyes are round and flash only while it speaks, and it wiggles as it does.
- The parachutes lean away from each other.
- The mission ends with its own message ("The delivery near X has self-destructed"), not
  Expansion's "destroyed by infected".
- `fallSpeedScale`: how fast the drop falls, times Expansion's speed (default 1).
- Upgrading: the server only writes `ShippingCrate.json` when there isn't one, so delete (or
  rename) the old file to get the new loads, guns and settings.

## 0.2 - 2026-11-12

- The drop, lit: red, green and white navigation lights under the plane's wingtips and tail;
  a spotlight under the falling container; and a runway edge light at each end of the roof
  whose orange beacon rotates, falling and landed.
- Self-destruct, to stop containers piling up: a 4 x 2 grid of IEDs on the roof with a teddy
  bear sitting on top. Ten minutes after landing (`selfDestructMinutes`), the bear's eyes glow
  red and it calls "Эвакуируйтесь!" ("Evacuate!") every 3 s. A minute later (`warningSeconds`)
  the roof blows, and the container and every vehicle and item inside it are removed.
  `selfDestructMinutes: -1` turns it off.
- Nothing on the roof or the plane can be picked up or used: it is all scenery.
- Notifications name the nearest town properly on maps whose town names aren't in the class
  names (Nasdara: "Dashtabad", not "2").
- Fixed: a crash when leaving the server with a drop nearby; a second container copy lingering
  underground after landing.

## 0.1 - 2026-11-12

First release.

- Expansion Missions airdrops drop a vanilla locked shipping container (doors unlocked) on the
  middle of the biggest group of players, on the nearest flat, clear ground.
- It falls under two parachutes, one over each end.
- No infected at the drop.
- The contents are a JSON file the server writes on its first start,
  `profiles/ShippingCrate/ShippingCrate.json`. The default load is a fitted-out, fuelled Ada
  with four SCR 17s (ACOG, stock, light, loaded magazine), eight spare magazines, a bottle of
  petrol, a lighter and a rope in the boot, and a hand truck on the floor.
- Player-called supply-signal airdrops are left alone.
