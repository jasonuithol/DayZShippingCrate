# Shipping Crate

Deliveries by shipping container, ordered on a radio. A DayZ mod for servers running DayZ
Expansion Missions: a player holds the delivery radio's button and one of Expansion's airdrop
planes heads for where they stood. What comes down under parachutes is a vanilla shipping
container, doors unlocked, loaded with whatever the config says. No infected at the drop.
Expansion's own airdrops (missions and supply signals) carry on as before, untouched.

The default loads are every vanilla car, truck and motorbike (Ada 4x4, Olga 24, Gunter 2,
Sarka 120, M1025, M3S Covered, Jana 50, Bitrak 682), each with every part and every fluid, and
nothing else. The cars come in the container, nose to the doors; the M3S and the M1025 are too
big for it and come down on a deck of wooden pallets. With DayZ Expansion Vehicles, its cars,
helicopters and boats are on the list too.

**Equipment** is the one load without a vehicle: a container holding a hand truck, a barrel of
guns (the five rifles: VSD, LAR, KA-M, SCR 16 and SVAL, loaded, with two spare magazines each), a
barrel of medical supplies and food, two barrels of clothing and gear, a barrel of base-building
tools and kits, a barrel of field gear (night-vision goggles, head torch, binoculars, lockpick,
screwdriver, batteries, every repair kit), a belt with a canteen, a sheathed knife and a
holster, a barrel of explosives (C4, IEDs and detonators, claymores, land mines, grenades and
smoke).

Players order with the **delivery radio**, a bright orange walkie-talkie every new character
starts with. In hand, a click steps its knob to the next vehicle ("any", a random one, sits
between the last and the first), and holding it (**Order drop here**) orders that vehicle
dropped where you stand. One delivery at a time per player: the next order waits until the
last one has landed (or, with the self-destruct on, blown up).

## How it works

1. **Where.** From where the player stood when they ordered, the mod searches outwards (up to
   `searchRadius`) for dry, flat (`maxSlope`), clear ground: no trees, buildings, walls, rocks,
   vehicles or players within 9 m. One of Expansion's airdrop planes (the kind a supply signal
   calls, using Expansion's airdrop settings for height and speed) is sent there. The player is
   told it's coming, named after the nearest town.
2. **The fall.** The plane drops Expansion's own crate (`ShippingCrate_Airdrop`), with no loot
   and no infected. Clients hide the crate and draw a shipping container in its place, with a
   parachute over each end.
   A vehicle too big for the container rides down for real on a pallet deck drawn under the
   crate instead.
3. **Landing.** When the crate comes to rest, the server places the real container
   (`Land_ContainerLocked_<colour>_DE`, the 1.23+ one with doors) or pallet deck and loads it. The crate itself
   is parked out of sight under the container floor until the self-destruct.

4. **Lights.** The plane carries red, green and white navigation lights (left wingtip, right
   wingtip, tail). A spotlight under the falling container lights the ground below, and a
   runway edge light at each end of its roof turns an orange beacon, falling and landed.
5. **Self-destruct (off by default).** With `selfDestructMinutes` set, a stack of IEDs with a
   teddy bear on top sits on the roof (or the pallet's free end). That many minutes after
   landing the bear's eyes glow red and it calls "Эвакуируйтесь!" ("Evacuate!")
   every 3 s, wiggling as it does; `warningSeconds` (60) later it blows (real C4, real damage) and the
   container and every vehicle and item in it are removed, the crate with them. That keeps
   containers from piling up, and with them memory. Everything on the roof and the plane is
   scenery: nothing can be picked up.

The container doesn't survive a server
restart (script-placed buildings aren't saved), and neither does what was on or in it: drops
that hadn't blown up yet are recorded in `profiles/ShippingCrate/landed.json` and cleared away
20 s after the next start.

## For server owners

1. Put the mod in the server folder as `@ShippingCrate` and load it on server **and** clients
   with `-mod=`, after Expansion. Players need it to see the falling container.
2. Requires `@DayZ-Expansion-Missions` (and what it needs: CF, Dabs Framework,
   Expansion-Licensed, Expansion-Core).
3. Start the server once. It writes the defaults to `profiles/ShippingCrate/ShippingCrate.json`;
   edit that and restart. It only writes the file when there isn't one: after an update, delete
   (or rename) it to get the new defaults.

### ShippingCrate.json

```json
{
    "enabled": true,
    "dropRadius": 1,
    "searchRadius": 50,
    "maxSlope": 8,
    "openDoors": false,
    "selfDestructMinutes": -1,
    "warningSeconds": 60,
    "fallSpeedScale": 1,
    "colours": ["Blue", "Yellow", "Orange", "Red"],
    "drops": [
        {
            "name": "Ada 4x4",
            "weight": 1,
            "carrier": "",
            "vehicle": {
                "type": "OffroadHatchback|OffroadHatchback_Blue|OffroadHatchback_White",
                "fluids": true,
                "attachments": [{"type": "HatchbackWheel", "count": 5}, {"type": "CarBattery"}],
                "cargo": [{"type": "GlassBottle", "liquid": "Gasoline"}]
            },
            "items": [{"type": "HandCart_HandTruck_Blue|HandCart_HandTruck_Red"}]
        }
    ],
    "guns": [
        {
            "name": "SCR 16",
            "weight": 1,
            "cargo": [
                {
                    "type": "SCARL|SCARL_Black", "count": 4, "magazine": "Mag_STANAG_30Rnd",
                    "attachments": [{"type": "ACOGOptic"}, {"type": "UniversalLight", "attachments": [{"type": "Battery9V"}]}]
                },
                {"type": "Mag_STANAG_30Rnd", "count": 8}
            ]
        }
    ]
}
```

| Field | What |
|---|---|
| `enabled` | `false` switches deliveries off: the radio's button just says so. Keep it in the file: a missing value reads as `false`. |
| `dropRadius` | Expansion's random spread around the drop spot (m). Wind drift grows with it too, so keep it small. |
| `searchRadius`, `maxSlope` | How far (m) from where the order was made to look for ground at most `maxSlope` degrees off level. |
| `openDoors` | The doors are always unlocked; `true` opens them as well. |
| `selfDestructMinutes` | `-1` (the default, also when left out): no self-destruct. The drop stays until the next restart and is cleared then, and the player can order again as soon as it has landed. A number of minutes turns it on: after that long the bear (on the roof, or the pallet's free end) starts warning, then it all blows up and is removed, and only then can the player order again. |
| `warningSeconds` | How long the bear warns before the blast. |
| `fallSpeedScale` | How fast the drop falls, times Expansion's speed. |
| `colours` | Which of the four vanilla containers to pick from: `Blue`, `Yellow`, `Orange`, `Red`. |
| `drops` | The possible loads: the delivery radio's knob steps through them by `name`. On "any", an order picks one at random, weighted by `weight`. |
| `drops[].needs` | Optional: a `CfgPatches` class the load needs. On a server without that mod the load is left out (the Expansion vehicles need `DayZExpansion_Vehicles_Scripts`). A vehicle class the server doesn't have is left out too. |
| `drops[].water` | `true`: a boat. It's dropped on open water (sea or pond, 1.5 m deep and clear for 8 m around) within `searchRadius` of the player, on a pallet that sinks to the bottom while the boat floats. No water that close: the order is refused. On "any", boats only come up where there's water for them. |
| `drops[].carrier` | `"container"`, `"pallet"`, or left empty to choose by the vehicle's size. |
| `drops[].vehicle` | Optional. In a container, parked front bumper 30 cm from the doors; on a pallet, at one end. |
| `drops[].items` | Loose on the floor behind the vehicle (or across the whole floor if there's none), three to a row; on a pallet, on its free end. |
| `guns` | Gun kits. Deliveries don't use them any more; `tools/make_loads.py` fills the Equipment load's gun barrel from them (each rifle kit's gun, with two spare magazines: a barrel holds five, so not the R12 or the M79). |

Every item, the vehicle included, takes:

| Field | What |
|---|---|
| `type` | Class name. `"A\|B\|C"` picks one at random. |
| `count` | How many. Left out: 1. |
| `attachments` | Items on it. Vehicle parts are fitted to whichever slot is free, so `HatchbackWheel` × 5 gives four wheels and the spare. |
| `cargo` | Items in it: a vehicle's boot, a backpack, a box. |
| `magazine` | Firearms: a full magazine loaded, a round chambered. |
| `liquid` | Bottles, canisters, pots: `Water`, `CleanWater`, `Gasoline` (or `Petrol`), `Diesel`, `Vodka`, `Beer`, `Disinfectant`, `Saline`. Filled to the brim unless `quantity` says otherwise. |
| `quantity` | 0..1 of the maximum (stack size, liquid, rounds). Left out: the game's default. |
| `fluids` | Vehicles: fill fuel, oil, coolant and brake fluid. |
| `fit` | Vehicles: fitted out by the vehicle's own debug-spawn routine first (wheels, parts, and doors that match its skin). The Expansion vehicles use this. |

Anything that can't be created, attached or fitted in is skipped with a `[ShippingCrate]` line in
the server's script log. If the file can't be read at all, airdrops stay vanilla Expansion and the
log says why.

The container is 6.26 m long inside and 2.48 m wide (doors at both ends): a vehicle whose body
is more than 5.3 m long, 2.0 m wide or 2.35 m tall goes on a pallet instead. Run
`tools/test.sh shippingcrate` after changing the loads: it spawns every vehicle with its parts,
fits every gun kit into every boot, and logs whatever didn't attach, load or fit.

`tools/make_loads.py` rebuilds the default `drops` and `guns` from vanilla's own spawn presets
(`cfgspawnabletypes.xml`) and a dump of the game config (`tools/data/config_dump.txt`).

## Layout

| Path | What |
|---|---|
| `ShippingCrate/` | The mod (PBO prefix `ShippingCrate`): config, scripts, the default `ShippingCrate.json` |
| `ShippingCrate/Scripts/4_World/ShippingCrate/SC_Airdrop.c` | The Expansion hooks; the crate class (server: landing; client: the container and parachutes drawn in its place) |
| `.../SC_Spot.c` | Grouping players, finding flat clear ground, the nearest town |
| `.../SC_Spawn.c` | Placing and loading the container or pallet |
| `.../SC_Pallet.c` | The pallet deck: its layout and the vehicle on it |
| `.../SC_DropRadio.c` | The delivery radio and its tuning actions (next, previous) |
| `.../SC_Leftovers.c` | Clearing away drops a server restart left standing |
| `ShippingCrate/Scripts/5_Mission/` | A delivery radio for every new character; the clean-up after a restart |
| `.../SC_Settings.c` | The JSON settings |
| `mission/` | Test mission on Chernarus |
| `.../SC_Lights.c` | The lights: navigation lights, spotlight, beacons, the bear's eyes |
| `ShippingCrate/data/` | Glowing materials (navigation markers, beacon dome) and the bear's voice |
| `tools/make_preview.py` | Draws the Workshop picture, `ShippingCrate/preview.png` |
| `tools/make_loads.py` | Rebuilds the default loads and gun kits from vanilla's data |
| `tools/make_sounds.py` | Speaks the bear's "Эвакуируйтесь!" with the espeak-ng library (packages `libespeak-ng1`, `espeak-ng-data`) |
| `tools/make_radio.py` | Makes the delivery radio's orange model from vanilla's walkie-talkie |
| `CHANGELOG.md` | One entry per release; the entry is also the Workshop change note |

## Testing

`mission/` is linked into DayZMissions as `missions/shippingcrate.chernarusplus`, and DayZMissions
`mods/ShippingCrate` is a symlink to `ShippingCrate/` here. From DayZMissions:

- `tools/test.sh shippingcrate` boots a headless server. It checks every class name in the
  config, spawns every vehicle and fits every gun kit, checks the radio's knob and the
  restart clean-up, and drops a real crate from 60 m on the fields west of
  Balota, then a pallet with the M3S. It logs what landed: container, doors, the vehicle's position and parts, the boot,
  the floor. Then it deletes the crate the way a mission end does, checks the container went
  with it, and shuts down (`[SCTest]` lines).
- `tools/run_server.sh shippingcrate` and `tools/run_client.sh shippingcrate` play it for real.
  Hold the radio to order a drop. To save waiting, the test mission flies Expansion's airdrop
  planes at 150 m/s instead of 25, so a delivery covers the ~16 km from the map edge in about 2
  minutes. Creating `profiles/sc_drop_now` starts one of Expansion's own airdrop missions, to
  check they're left alone. `profiles/sc_tp` (optionally holding "x z") moves everyone onto
  dry land north of Elektrozavodsk; `profiles/sc_spawn` holding a class name (default
  `ExpansionUh1h`) spawns a fitted vehicle 15 m in front of the first player, clearing earlier
  ones of that type around the spot first. Once a second, `[SCFlight]` lines log every player's
  vehicle: position, height above ground, speed, climb and attitude. The mission also loads Expansion Navigation for the map on M. The delivery plane's height and speed come from Expansion's
  `profiles/ExpansionMod/Settings/AirdropSettings.json`.
