#!/usr/bin/env python3
"""Rebuild the "drops" (vehicles) and "guns" (kits) in ShippingCrate/ShippingCrate.json from
vanilla's own data: the server's cfgspawnabletypes.xml (which parts a vehicle spawns with,
which attachments a gun does) and a dump of the game config (each gun's display name,
magazines and ammo: tools/data/config_dump.txt, printed from a headless server as
"[SCDUMP] W|class|displayName|scope|isPistol|magazines,|chamberableFrom,"). Keeps the other
settings, and the kits already in the file for KEEP's guns.

    tools/make_loads.py <cfgspawnabletypes.xml> tools/data/config_dump.txt ShippingCrate/ShippingCrate.json
"""
import json, re, sys

spawnable_path, dump_path, json_path = sys.argv[1:4]
spawnable = open(spawnable_path).read()

def presets(name):
    """[[item, ...] per attachment group] from cfgspawnabletypes, or []"""
    m = re.search(r'<type name="%s">(.*?)</type>' % re.escape(name), spawnable, re.S)
    if not m:
        return []
    return [re.findall(r'item name="([^"]+)"', g) for g in re.findall(r'<attachments[^>]*>(.*?)</attachments>', m.group(1), re.S)]

def parts(name):
    return [i for g in presets(name) for i in g]

# ---- vehicles -------------------------------------------------------------------------
# name, colour variants, extra boot contents, items on the floor / deck
LOOSE = [{"type": "HandCart_HandTruck_Blue|HandCart_HandTruck_Red|HandCart_HandTruck_Yellow"}]
# the sea chest: every vanilla explosive that isn't ammunition, and detonators to set off the
# C4 and IEDs
EXPLOSIVES = [("RemoteDetonator", 2), ("Plastic_Explosive", 3), ("ImprovisedExplosive", 2), ("ClaymoreMine", 2),
              ("LandMineTrap", 2), ("M67Grenade", 3), ("RGD5Grenade", 3), ("FlashGrenade", 2), ("Grenade_ChemGas", 1),
              ("M18SmokeGrenade_Green", 1), ("M18SmokeGrenade_Purple", 1)]   # the chest is full: two smokes, not every colour
SEA_CHEST = {"type": "SeaChest", "cargo": [dict(type=t, **({"count": n} if n > 1 else {})) for t, n in EXPLOSIVES]}
TRUCK_LOOSE = LOOSE + [SEA_CHEST, {"type": "Barrel_Blue|Barrel_Green|Barrel_Red|Barrel_Yellow"}]
BOOT = [{"type": "GlassBottle", "liquid": "Gasoline", "quantity": 1}, {"type": "PetrolLighter"}, {"type": "Rope"}]
VEHICLES = [
    ("Ada 4x4", ["OffroadHatchback", "OffroadHatchback_Blue", "OffroadHatchback_White"], BOOT, LOOSE),
    ("Olga 24", ["CivilianSedan", "CivilianSedan_Black", "CivilianSedan_Wine"], BOOT, LOOSE),
    ("Gunter 2", ["Hatchback_02", "Hatchback_02_Black", "Hatchback_02_Blue"], BOOT, LOOSE),
    ("Sarka 120", ["Sedan_02", "Sedan_02_Grey", "Sedan_02_Red"], BOOT, LOOSE),
    ("M1025", ["Offroad_02", "Offroad_02_Beige"], BOOT, LOOSE),
    ("M3S Covered", ["Truck_01_Covered", "Truck_01_Covered_Blue", "Truck_01_Covered_Orange"], [], TRUCK_LOOSE),
    # no boot: the gun kit lies on the floor beside the bike
    ("Motorbike", ["Motorbike_01_Blue", "Motorbike_01_Red", "Motorbike_01_Yellow"], [], LOOSE),
    ("Sport bike", ["Motorbike_02_Red"], [], LOOSE),
]

def counted(types):
    out = []
    for t in types:
        if out and out[-1]["type"] == t:
            out[-1]["count"] = out[-1].get("count", 1) + 1
        else:
            out.append({"type": t})
    return out

drops = []
for name, variants, boot, loose in VEHICLES:
    p = parts(variants[0])
    if not p:
        sys.exit("no parts for " + variants[0])
    vehicle = {"type": "|".join(variants), "fluids": True, "attachments": counted(p), "cargo": boot}
    drops.append({"name": name, "weight": 1, "carrier": "", "vehicle": vehicle, "items": loose})

# ---- guns ---------------------------------------------------------------------------------
guns = {}
order = []
for line in open(dump_path):
    f = line.rstrip("\n").split("|")
    if len(f) < 7 or f[0] != "[SCDUMP] W":
        continue
    cls, display, scope, pistol, mags, ammo = f[1], f[2], f[3], f[4], [m for m in f[5].split(",") if m], [a for a in f[6].split(",") if a]
    guns[cls] = dict(display=display, pistol=pistol == "1", mags=mags, ammo=ammo)
    order.append(cls)

SKIP = re.compile(r"^(Sawedoff|Flaregun)|_Gold$")
COLOUR = re.compile(r"_(Black|Green|Woodland|Desert|Winter|Camo|Tan|Grey|Pink|Autumn|Summer|Wood|Wooden|Chernarus|Livonia)$")
# the standard magazine, where the first in the config isn't it
MAG = {"AKM": "Mag_AKM_30Rnd", "M4A1": "Mag_STANAG_30Rnd", "M16A2": "Mag_STANAG_30Rnd", "SCARL": "Mag_STANAG_30Rnd",
       "Aug": "Mag_Aug_30Rnd", "CZ550": "Mag_CZ550_10rnd", "MP5K": "Mag_MP5_30Rnd", "PM73Rak": "Mag_PM73_25Rnd",
       "Saiga": "Mag_Saiga_8Rnd", "ASVAL": "Mag_VAL_20Rnd", "Vikhr": "Mag_Vikhr_30Rnd", "Ruger1022": "Mag_Ruger1022_30Rnd",
       "M14": "Mag_M14_20Rnd", "Luger": "Mag_Luger_8Rnd", "MP18": "Mag_MP18_Drum32Rnd", "AK74": "Mag_AK74_30Rnd",
       "AKS74U": "Mag_AK74_30Rnd", "SCARH": "Mag_SCARH_20Rnd"}
BOX = {"Ammo_556x45": "AmmoBox_556x45_20Rnd", "Ammo_545x39": "AmmoBox_545x39_20Rnd", "Ammo_762x39": "AmmoBox_762x39_20Rnd",
       "Ammo_762x54": "AmmoBox_762x54_20Rnd", "Ammo_308Win": "AmmoBox_308Win_20Rnd", "Ammo_357": "AmmoBox_357_20Rnd",
       "Ammo_9x19": "AmmoBox_9x19_25rnd", "Ammo_380": "AmmoBox_380_35rnd", "Ammo_45ACP": "AmmoBox_45ACP_25rnd",
       "Ammo_22": "AmmoBox_22_50Rnd", "Ammo_9x39": "AmmoBox_9x39_20Rnd", "Ammo_12gaPellets": "AmmoBox_00buck_10rnd"}
# loose rounds for what has no ammo box
LOOSE_AMMO = {"Ammo_HuntingBolt": 12, "Ammo_40mm_Explosive": 12}
# the four kits there were before: kept as they were (the SCR 17's ACOG etc.)
KEEP = {"SCARH", "M4A1", "AKM", "SVD"}

# Not every gun: the hardest-hitting of each calibre (no pistols, no pistol-calibre SMGs).
WANT = [
    "SVD",     # 7.62x54R
    "FAL",     # .308
    "AKM",     # 7.62x39
    "SCARL",   # 5.56x45
    "ASVAL",   # 9x39
    "R12",     # 12 gauge
    "M79",     # 40 mm grenades
]

old = json.load(open(json_path))
old_guns = {g["cargo"][0]["type"].split("|")[0]: g for g in old.get("guns", [])}

kits = []
seen = set()
for cls in order:
    if SKIP.search(cls):
        continue
    base = COLOUR.sub("", cls)
    if base not in guns:
        base = cls
    if base in seen:
        continue
    if base not in WANT:
        continue
    seen.add(base)
    variants = [c for c in order if c == base or (COLOUR.sub("", c) == base and not SKIP.search(c))]
    g = guns[base]
    if base in KEEP and base in old_guns:
        kit = old_guns[base]
        kit["name"] = g["display"]
        kits.append(kit)
        continue
    if base == "SCARL":
        # no vanilla preset: stock, ACOG, light with a battery
        atts_h = [{"type": "SCAR_StockBttstck"}, {"type": "ACOGOptic"}, {"type": "UniversalLight", "attachments": [{"type": "Battery9V"}]}]
        kits.append({"name": g["display"], "weight": 1, "cargo": [
            {"type": "|".join(variants), "count": 4, "magazine": "Mag_STANAG_30Rnd", "attachments": atts_h},
            {"type": "Mag_STANAG_30Rnd", "count": 8}]})
        continue
    mag = MAG.get(base, g["mags"][0] if g["mags"] else "")
    atts = []
    for group in presets(base):
        group = [i for i in group if not i.startswith("Mag_")]
        if group:
            att = {"type": group[0]}
            if group[0] in ("UniversalLight", "TLRLight"):
                att["attachments"] = [{"type": "Battery9V"}]   # a light that works
            atts.append(att)
    gun = {"type": "|".join(variants), "count": 4}
    if mag:
        gun["magazine"] = mag
    if atts:
        gun["attachments"] = atts
    cargo = [gun]
    if mag:
        cargo.append({"type": mag, "count": 8})
    else:
        rounds = g["ammo"][0]
        if rounds in BOX:
            cargo.append({"type": BOX[rounds], "count": 6})
        elif rounds in LOOSE_AMMO:
            cargo.append({"type": rounds, "count": LOOSE_AMMO[rounds]})
        else:
            sys.exit("no ammo for " + base + ": " + rounds)
    kits.append({"name": g["display"], "weight": 1, "cargo": cargo})

# the same display name twice (the crossbows): keep the first
names = set()
kits = [k for k in kits if not (k["name"] in names or names.add(k["name"]))]

# pistols come up less often when nobody's radio picks the guns
for k in kits:
    base = k["cargo"][0]["type"].split("|")[0]
    if guns.get(base, {}).get("pistol"):
        k["weight"] = 0.3
kits.sort(key=lambda k: WANT.index(k["cargo"][0]["type"].split("|")[0]))

old["drops"] = drops
old["guns"] = kits
json.dump(old, open(json_path, "w"), indent=4, ensure_ascii=False)
open(json_path, "a").write("\n")
print(len(drops), "vehicles:", ", ".join(d["name"] for d in drops))
print(len(kits), "gun kits:", ", ".join(k["name"] for k in kits))
