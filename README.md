# Shipping Crate

Expansion's airdrops, delivered by shipping container. A DayZ mod that hijacks DayZ Expansion
Missions' airdrops: the plane heads for the middle of the biggest group of players instead of a
fixed town, and what comes down under two parachutes is a vanilla shipping container, doors
unlocked, loaded with whatever the config says. No infected at the drop.

The default loads are every vanilla car, truck and motorbike (Ada 4x4, Olga 24, Gunter 2,
Sarka 120, M1025, M3S Covered, Jana 50, Bitrak 682), each with every part and every fluid. The
cars come in the container, nose to the doors, with a bottle of petrol, a lighter and a rope in
the boot and a hand truck behind. The M3S and the M1025 are too big for it and come down on a deck
of wooden pallets, the M3S with a sea chest full of explosives and a barrel as well. In the boot
go the guns: a kit of four (VSD, LAR, KA-M, SCR 16, SVAL, R12 or M79, loaded, with spare
magazines), or, if nobody chose, one of each until the boot is full.

Players pick what comes with the **delivery radio**, a bright orange walkie-talkie every new
character starts with: one knob for the vehicle (click), one for the guns (hold). The next drop
on a group with a tuned radio brings what it's tuned to.

## How it works

1. **Where.** When an Expansion airdrop *mission* starts, the mod groups the living players
   (everyone within `clusterRadius` of each other) and takes the centre of the biggest group.
   From there it searches outwards for dry, flat (`maxSlope`), clear ground: no trees,
   buildings, walls, rocks, vehicles or players within 9 m. The mission's drop location moves
   there, named after the nearest town for Expansion's notifications. If nobody is online, it's
   a normal Expansion airdrop.
2. **The fall.** The plane drops Expansion's own crate (`ShippingCrate_Airdrop`), with no loot
   and no infected. Clients hide the crate and draw a shipping container in its place, with a
   parachute over each end.
   A vehicle too big for the container rides down for real on a pallet deck drawn under the
   crate instead.
3. **Landing.** When the crate comes to rest, the server places the real container
   (`Land_ContainerLocked_<colour>_DE`, the 1.23+ one with doors) or pallet deck and loads it. The crate itself
   is parked out of sight under the container floor, so Expansion's own rule still ends the
   mission: time up, and nobody within 1100 m. When Expansion deletes the crate, the container
   goes with it. The vehicle and everything else stays, like any other loot.

4. **Lights.** The plane carries red, green and white navigation lights (left wingtip, right
   wingtip, tail). A spotlight under the falling container lights the ground below, and a
   runway edge light at each end of its roof turns an orange beacon, falling and landed.
5. **Self-destruct.** On the roof (or the pallet's free end) sits a stack of IEDs with a teddy
   bear on top. `selfDestructMinutes`
   (10) after landing the bear's eyes glow red and it calls "Эвакуируйтесь!" ("Evacuate!")
   every 3 s, wiggling as it does; `warningSeconds` (60) later it blows (real C4, real damage) and the
   container and every vehicle and item in it are removed, the mission with them. That keeps
   containers from piling up, and with them memory. Everything on the roof and the plane is
   scenery: nothing can be picked up.

Player-called airdrops (supply signals) are left alone. The container doesn't survive a server
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
    "clusterRadius": 1000,
    "dropRadius": 1,
    "searchRadius": 200,
    "maxSlope": 8,
    "openDoors": false,
    "selfDestructMinutes": 10,
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
| `enabled` | `false` leaves every airdrop to Expansion. Keep it in the file: a missing value reads as `false`. |
| `clusterRadius` | Players this close (m) to each other count as one group. |
| `dropRadius` | Expansion's random spread around the drop spot (m). Wind drift grows with it too, so keep it small. |
| `searchRadius`, `maxSlope` | How far (m) from the group's centre to look for ground at most `maxSlope` degrees off level. |
| `openDoors` | The doors are always unlocked; `true` opens them as well. |
| `selfDestructMinutes` | Minutes after landing until the bear starts warning; then it all blows up and is removed. `-1`: never (the container then goes when Expansion ends the mission). |
| `warningSeconds` | How long the bear warns before the blast. |
| `fallSpeedScale` | How fast the drop falls, times Expansion's speed. |
| `colours` | Which of the four vanilla containers to pick from: `Blue`, `Yellow`, `Orange`, `Red`. |
| `drops` | The possible loads: the delivery radio's vehicle knob steps through them by `name`. Untuned, a drop picks one at random, weighted by `weight`. |
| `drops[].carrier` | `"container"`, `"pallet"`, or left empty to choose by the vehicle's size. |
| `drops[].vehicle` | Optional. In a container, parked front bumper 30 cm from the doors; on a pallet, at one end. |
| `drops[].items` | Loose on the floor behind the vehicle (or across the whole floor if there's none), three to a row; on a pallet, on its free end. |
| `guns` | The gun kits: the radio's guns knob steps through them by `name`. A kit's `cargo` goes in the vehicle's boot (on the floor if it has none). Untuned ("any"), the boot gets one of each kit's guns with a quarter of its spare ammunition, over and over until it's full. |

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
| `.../SC_DropRadio.c` | The delivery radio and its two tuning actions |
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
  config, spawns every vehicle and fits every gun kit, checks the radio's knobs, the grouping
  maths and the restart clean-up, and drops a real crate from 60 m on the fields west of
  Balota, then a pallet with the M3S. It logs what landed: container, doors, the vehicle's position and parts, the boot,
  the floor. Then it deletes the crate the way a mission end does, checks the container went
  with it, and shuts down (`[SCTest]` lines).
- `tools/run_server.sh shippingcrate` and `tools/run_client.sh shippingcrate` play it for real.
  To save waiting, the test mission starts an airdrop 5 s after the first player logs in, then
  another every 10 minutes. Each uses a different Expansion airdrop mission, they can overlap,
  and the plane flies at 150 m/s instead of 25, so it covers the ~16 km from the map edge in
  about 2 minutes. The mission also loads Expansion Navigation for the map on M. On a normal
  server, Expansion's first airdrop comes 5 minutes after start, then one an hour
  (`profiles/ExpansionMod/Settings/MissionSettings.json`).
