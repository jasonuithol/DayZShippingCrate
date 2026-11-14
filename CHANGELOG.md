# Changelog

## 0.5.1 - 2026-11-14

- The airdrop planes' navigation lights are off: game crashes (the same access violation every
  time) followed planes appearing on players' games, Expansion's own airdrops included.
- Every player always has a delivery radio. A new character whose outfit has no room for it
  (a nurse, say) now has things thrown out of their pockets onto the ground, one at a time,
  until it fits, instead of going without; only with nowhere at all to carry it does it go in
  hand (or at their feet). Anyone joining without one is given another the same way.

## 0.5 - 2026-11-14

- Deliveries are ordered on the delivery radio, and nothing else starts them: hold it
  ("Order drop here") and one of Expansion's airdrop planes heads for flat, clear ground where
  you stood, bringing the vehicle the knob is on. One delivery at a time per player, until the
  last one has blown up. Expansion's own airdrops (missions and supply signals) are no longer
  touched at all. `clusterRadius` is gone, and `searchRadius` is now 50 m.
- The delivery radio has one knob, for the vehicle: click for the next. "any" (a random vehicle)
  is a stop on the dial, between the last and the first. The guns knob is gone.
- The Tractor and the Gyrocopter come down on pallets.
- Vehicles come alone: no guns in the boot, no petrol, lighter or rope, no hand truck, sea
  chest or barrel beside them.
- New load, Equipment (first on the radio): a container with a hand truck, a barrel of guns
  (the five rifles, loaded, two spare magazines each), a barrel of medical supplies and food, two
  barrels of clothing and gear, a barrel of base-building tools and kits, a barrel of field gear
  (night-vision goggles, head torch, binoculars, lockpick, screwdriver, batteries, every repair
  kit), a belt with a canteen, a sheathed knife and a holster, a barrel of explosives (C4,
  IEDs and detonators, claymores, land mines, grenades, smoke).
- Boats: the Zodiac and the Utility Boat (with DayZ Expansion Vehicles). Ordered within 50 m
  (`searchRadius`) of open water, they come down on a pallet onto the water nearest the player:
  the pallet sinks to the bottom and the boat floats, fitted out and fuelled, with nothing else.
  No water nearby: the order is refused with a message. On "any", a boat only comes up where
  there's water for it. New load field `"water": true`.
- No self-destruct by default (`selfDestructMinutes` -1, also when left out): with deliveries on
  demand, drops stay until the next restart and are cleared then, and the next order can go as
  soon as the last one has landed. The bear, the IEDs and the blast only appear when
  `selfDestructMinutes` is set.

## 0.4 - 2026-11-13

- The Expansion vehicles are on the delivery radio, on servers running DayZ Expansion Vehicles:
  Land Rover, UAZ (and its Cargo and Roofless versions), Vodnik, Bus, Tractor, Gyrocopter, MH-6,
  UH-1H and Merlin, in all their skins, fitted out by Expansion's own routine (`"fit": true`) and
  fuelled. Boats are left out. A server without Expansion Vehicles leaves these loads out
  (`"needs"`), and any load whose vehicle class it hasn't got.
- Delivered vehicles no longer vanish. Putting anything in a vehicle (or on it) resets its
  lifetime to the economy's, which is 3 seconds for vanilla vehicles, and the economy then
  deleted it as soon as nobody was near: a car whose boot had just been filled with guns went
  within a minute of landing. Its lifetime is now set as a maximum, so it lasts 45 days.
- The self-destruct destroys everything in the container or on the pallet: the walls no longer
  shield the car inside. Each thing takes the C4's hit directly (a fuelled vehicle goes up, as
  any explosive sets it off), whatever survives is destroyed a moment later, and then it is all
  removed as before.

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
