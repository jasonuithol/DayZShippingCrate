# Changelog

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
