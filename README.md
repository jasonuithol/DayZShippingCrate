# Shipping Crate

Expansion's airdrops, delivered by shipping container. A DayZ mod that hijacks DayZ Expansion
Missions' airdrops: the plane heads for the middle of the biggest group of players instead of a
fixed town, and what comes down under two parachutes is a vanilla shipping container, doors
unlocked, loaded with whatever the config says. No infected at the drop.

The default load: an Ada (`OffroadHatchback`) with every part and every fluid, nose to the doors.
In its boot are four SCR 17s (`SCARH`: stock, ACOG, light with battery, loaded magazine), eight
spare magazines, a glass bottle of petrol, a lighter and a rope. On the floor behind it is a hand
truck.

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
3. **Landing.** When the crate comes to rest, the server places the real container
   (`Land_ContainerLocked_<colour>_DE`, the 1.23+ one with doors) and loads it. The crate itself
   is parked out of sight under the container floor, so Expansion's own rule still ends the
   mission: time up, and nobody within 1100 m. When Expansion deletes the crate, the container
   goes with it. The vehicle and everything else stays, like any other loot.

4. **Lights.** The plane carries red, green and white navigation lights (left wingtip, right
   wingtip, tail). A spotlight under the falling container lights the ground below, and a
   runway edge light at each end of its roof turns an orange beacon, falling and landed.
5. **Self-destruct.** On the roof sit a 4 x 2 grid of IEDs and a teddy bear. `selfDestructMinutes`
   (10) after landing the bear's eyes glow red and it calls "Эвакуируйтесь!" ("Evacuate!")
   every 3 s; `warningSeconds` (60) later the roof blows (real C4, real damage) and the
   container and every vehicle and item in it are removed, the mission with them. That keeps
   containers from piling up, and with them memory. Everything on the roof and the plane is
   scenery: nothing can be picked up.

Player-called airdrops (supply signals) are left alone. The container doesn't survive a server
restart (script-placed buildings aren't saved); what was in it does.

## For server owners

1. Put the mod in the server folder as `@ShippingCrate` and load it on server **and** clients
   with `-mod=`, after Expansion. Players need it to see the falling container.
2. Requires `@DayZ-Expansion-Missions` (and what it needs: CF, Dabs Framework,
   Expansion-Licensed, Expansion-Core).
3. Start the server once. It writes the defaults to `profiles/ShippingCrate/ShippingCrate.json`;
   edit that and restart.

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
    "colours": ["Blue", "Yellow", "Orange", "Red"],
    "drops": [
        {
            "name": "Ada and SCRs",
            "weight": 1,
            "vehicle": {
                "type": "OffroadHatchback",
                "fluids": true,
                "attachments": [{"type": "HatchbackWheel", "count": 5}, {"type": "CarBattery"}],
                "cargo": [
                    {
                        "type": "SCARH", "count": 4, "magazine": "Mag_SCARH_20Rnd",
                        "attachments": [{"type": "ACOGOptic"}, {"type": "UniversalLight", "attachments": [{"type": "Battery9V"}]}]
                    },
                    {"type": "GlassBottle", "liquid": "Gasoline"}
                ]
            },
            "items": [{"type": "HandCart_HandTruck_Blue|HandCart_HandTruck_Red"}]
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
| `colours` | Which of the four vanilla containers to pick from: `Blue`, `Yellow`, `Orange`, `Red`. |
| `drops` | The possible loads. Each drop picks one at random, weighted by `weight`. |
| `drops[].vehicle` | Optional. Parked inside, front bumper 30 cm from the doors. |
| `drops[].items` | Loose on the floor behind the vehicle (or across the whole floor if there's none), three to a row. |

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

The container is 6.26 m long inside and 2.48 m wide (doors at both ends), so the vehicle should
be car-sized. The Ada fits with about a metre to spare behind it, enough for the hand truck.
Anything else is untested: run `tools/test.sh shippingcrate` after changing the vehicle and
check where it ended up.

## Layout

| Path | What |
|---|---|
| `ShippingCrate/` | The mod (PBO prefix `ShippingCrate`): config, scripts, the default `ShippingCrate.json` |
| `ShippingCrate/Scripts/4_World/ShippingCrate/SC_Airdrop.c` | The Expansion hooks; the crate class (server: landing; client: the container and parachutes drawn in its place) |
| `.../SC_Spot.c` | Grouping players, finding flat clear ground, the nearest town |
| `.../SC_Spawn.c` | Placing and loading the container |
| `.../SC_Settings.c` | The JSON settings |
| `mission/` | Test mission on Chernarus |
| `.../SC_Lights.c` | The lights: navigation lights, spotlight, beacons, the bear's eyes |
| `ShippingCrate/data/` | Glowing materials (navigation markers, beacon dome) and the bear's voice |
| `tools/make_preview.py` | Draws the Workshop picture, `ShippingCrate/preview.png` |
| `tools/make_sounds.py` | Speaks the bear's "Эвакуируйтесь!" with the espeak-ng library (packages `libespeak-ng1`, `espeak-ng-data`) |
| `CHANGELOG.md` | One entry per release; the entry is also the Workshop change note |

## Testing

`mission/` is linked into DayZMissions as `missions/shippingcrate.chernarusplus`, and DayZMissions
`mods/ShippingCrate` is a symlink to `ShippingCrate/` here. From DayZMissions:

- `tools/test.sh shippingcrate` boots a headless server. It checks every class name in the
  config, checks the grouping maths, and drops a real crate from 60 m on the fields west of
  Balota. It logs what landed: container, doors, the vehicle's position and parts, the boot,
  the floor. Then it deletes the crate the way a mission end does, checks the container went
  with it, and shuts down (`[SCTest]` lines).
- `tools/run_server.sh shippingcrate` and `tools/run_client.sh shippingcrate` play it for real.
  To save waiting, the test mission starts an airdrop 5 s after the first player logs in, then
  another every 10 minutes. Each uses a different Expansion airdrop mission, they can overlap,
  and the plane flies at 150 m/s instead of 25, so it covers the ~16 km from the map edge in
  about 2 minutes. The mission also loads Expansion Navigation for the map on M. On a normal
  server, Expansion's first airdrop comes 5 minutes after start, then one an hour
  (`profiles/ExpansionMod/Settings/MissionSettings.json`).
