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
# name, colour variants (vehicles come alone: no extra cargo in them or beside them)
VEHICLES = [
    ("Ada 4x4", ["OffroadHatchback", "OffroadHatchback_Blue", "OffroadHatchback_White"]),
    ("Olga 24", ["CivilianSedan", "CivilianSedan_Black", "CivilianSedan_Wine"]),
    ("Gunter 2", ["Hatchback_02", "Hatchback_02_Black", "Hatchback_02_Blue"]),
    ("Sarka 120", ["Sedan_02", "Sedan_02_Grey", "Sedan_02_Red"]),
    ("M1025", ["Offroad_02", "Offroad_02_Beige"]),
    ("M3S Covered", ["Truck_01_Covered", "Truck_01_Covered_Blue", "Truck_01_Covered_Orange"]),
    ("Motorbike", ["Motorbike_01_Blue", "Motorbike_01_Red", "Motorbike_01_Yellow"]),
    ("Sport bike", ["Motorbike_02_Red"]),
]

# Expansion Vehicles' (and Expansion Licensed's): no cfgspawnabletypes entries, so they're
# fitted by their own debug-spawn routine ("fit"), which also gives each skin its matching
# doors. A server without the mod skips these loads (SC_Settings drops a vehicle it hasn't
# got). Boats are dropped on water near the player ("water").
EXPANSION = [
    ("Land Rover", ["Expansion_Landrover", "Expansion_Landrover_Green", "Expansion_Landrover_Red", "Expansion_Landrover_Grey",
                    "Expansion_Landrover_Blue", "Expansion_Landrover_Orange"]),
    ("UAZ", ["ExpansionUAZ", "ExpansionUAZ_Green", "ExpansionUAZ_Black", "ExpansionUAZ_LightGreen"]),
    ("UAZ Cargo", ["ExpansionUAZCargo", "ExpansionUAZCargo_Green", "ExpansionUAZCargo_Black", "ExpansionUAZCargo_LightGreen"]),
    ("UAZ Roofless", ["ExpansionUAZRoofless", "ExpansionUAZRoofless_Green", "ExpansionUAZRoofless_Black",
                      "ExpansionUAZRoofless_LightGreen"]),
    ("Vodnik", ["ExpansionVodnik", "ExpansionVodnik_Camo", "ExpansionVodnik_CamoMedical"]),
    ("Bus", ["ExpansionBus", "ExpansionBus_Blue", "ExpansionBus_Red", "ExpansionBus_Weeb"]),
    # these two fit the container, but belong on a pallet
    ("Tractor", ["ExpansionTractor", "ExpansionTractor_Red", "ExpansionTractor_Blue", "ExpansionTractor_Green"], "pallet"),
    ("Gyrocopter", ["ExpansionGyrocopter"], "pallet"),   # fits the container too, but flies off a pallet
    ("MH-6", ["ExpansionMh6", "ExpansionMh6_Green", "ExpansionMh6_Black", "ExpansionMh6_Blueline", "ExpansionMh6_Digital",
              "ExpansionMh6_Elliptical", "ExpansionMh6_Furious", "ExpansionMh6_GrayWatcher", "ExpansionMh6_Jeans",
              "ExpansionMh6_Resistance", "ExpansionMh6_Shadow", "ExpansionMh6_Sheriff", "ExpansionMh6_Speedy",
              "ExpansionMh6_Sunset", "ExpansionMh6_Vrana", "ExpansionMh6_Wasp", "ExpansionMh6_Wave"]),
    ("UH-1H", ["ExpansionUh1h", "ExpansionUh1h_Green", "ExpansionUh1h_Civilian", "ExpansionUh1h_Medical",
               "ExpansionUh1h_Uber", "ExpansionUh1h_Resistance"]),
    ("Merlin", ["ExpansionMerlin", "ExpansionMerlin_Green", "ExpansionMerlin_Dahoman", "ExpansionMerlin_Ion",
                "ExpansionMerlin_President", "ExpansionMerlin_Vrana", "ExpansionMerlin_Wave"]),
    # boats: dropped on water near the player, on a pallet that sinks; nothing else comes with them
    ("Zodiac", ["ExpansionZodiacBoat", "ExpansionZodiacBoat_Orange", "ExpansionZodiacBoat_Green",
                "ExpansionZodiacBoat_Black"], "pallet", "water"),
    ("Utility Boat", ["ExpansionUtilityBoat", "ExpansionUtilityBoat_Black", "ExpansionUtilityBoat_Green"], "pallet", "water"),
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
for name, variants in VEHICLES:
    p = parts(variants[0])
    if not p:
        sys.exit("no parts for " + variants[0])
    vehicle = {"type": "|".join(variants), "fluids": True, "attachments": counted(p), "cargo": []}
    drops.append({"name": name, "weight": 1, "carrier": "", "vehicle": vehicle, "items": []})
for name, variants, *rest in EXPANSION:
    vehicle = {"type": "|".join(variants), "fluids": True, "fit": True, "attachments": [], "cargo": []}
    # Expansion Licensed has some of these without Expansion Vehicles, but not the scripts that fit them out
    drop = {"name": name, "weight": 1, "carrier": rest[0] if rest else "", "needs": "DayZExpansion_Vehicles_Scripts", "vehicle": vehicle, "items": []}
    if "water" in rest:
        drop["water"] = True
    drops.append(drop)

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

# ---- equipment: no vehicle, a container of supplies ------------------------------------------
def barrel(colour, contents):
    return {"type": "Barrel_" + colour, "cargo": [dict(type=t, **({"count": n} if n > 1 else {}), **extra)
                                                  for t, n, extra in contents]}
GUN_BARREL_SKIP = {"R12", "M79"}   # a barrel holds the five rifles with their magazines, no more
def gun_barrel():
    # one of every rifle kit's gun, loaded, with two spare magazines
    out = []
    for k in kits:
        if k["cargo"][0]["type"].split("|")[0] in GUN_BARREL_SKIP:
            continue
        gun = dict(k["cargo"][0]); gun.pop("count", None)
        out.append((gun.pop("type"), 1, gun))
        for c in k["cargo"][1:2]:
            out.append((c["type"], 2 if c["type"].startswith("Mag_") else min(c.get("count", 1), 12), {}))
    return out
EQUIPMENT = {"name": "Equipment", "weight": 1, "carrier": "container", "items": [
    {"type": "HandCart_HandTruck_Blue|HandCart_HandTruck_Red|HandCart_HandTruck_Yellow"},
    barrel("Green", gun_barrel()),
    barrel("Red", [("BandageDressing", 4, {}), ("SalineBagIV", 2, {}), ("Morphine", 2, {}), ("Epinephrine", 2, {}),
                   ("VitaminBottle", 1, {}), ("TetracyclineAntibiotics", 1, {}), ("CharcoalTablets", 1, {}),
                   ("PainkillerTablets", 1, {}), ("BakedBeansCan", 3, {}), ("TacticalBaconCan", 3, {}),
                   ("SardinesCan", 2, {}), ("PeachesCan", 2, {}), ("WaterBottle", 2, {"liquid": "Water", "quantity": 1}),
                   ("CanOpener", 1, {})]),
    barrel("Blue", [("GorkaEJacket_Summer", 1, {}), ("GorkaPants_Summer", 1, {}), ("MilitaryBoots_Black", 1, {}),
                    ("TacticalGloves_Black", 1, {}), ("BallisticHelmet_Black", 1, {}), ("PlateCarrierVest", 1, {}),
                    ("MilitaryBelt", 1, {}), ("Balaclava3Holes_Black", 1, {}), ("AviatorGlasses", 1, {})]),
    barrel("Blue", [("AliceBag_Green", 1, {}), ("SmershVest", 1, {}), ("M65Jacket_Black", 1, {}), ("CargoPants_Black", 1, {}),
                    ("CombatBoots_Black", 1, {}), ("WorkingGloves_Black", 1, {}), ("BaseballCap_Black", 1, {}),
                    ("ChestHolster", 1, {}), ("HuntingJacket_Autumn", 1, {}), ("HunterPants_Autumn", 1, {})]),
    barrel("Yellow", [("Hammer", 1, {}), ("Hatchet", 1, {}), ("Shovel", 1, {}), ("Pliers", 1, {}), ("Hacksaw", 1, {}),
                      ("NailBox", 3, {}), ("MetalWire", 2, {}), ("BarbedWire", 1, {}), ("CombinationLock4", 1, {}),
                      ("FenceKit", 2, {}), ("WatchtowerKit", 1, {}), ("TerritoryFlagKit", 1, {}), ("Rope", 1, {})]),
    barrel("Green", [("NVGoggles", 1, {"attachments": [{"type": "Battery9V"}]}),
                     ("Headtorch_Black", 1, {"attachments": [{"type": "Battery9V"}]}), ("Battery9V", 4, {}),
                     ("Binoculars", 1, {}), ("Lockpick", 1, {}), ("Screwdriver", 1, {}), ("SewingKit", 1, {}),
                     ("LeatherSewingKit", 1, {}), ("WeaponCleaningKit", 1, {}), ("ElectronicRepairKit", 1, {}),
                     ("TireRepairKit", 1, {}), ("EpoxyPutty", 1, {})]),
    # thrown and deployable explosives, with detonators for the C4 and the IEDs
    barrel("Red", [("RemoteDetonator", 2, {}), ("Plastic_Explosive", 3, {}), ("ImprovisedExplosive", 2, {}),
                   ("ClaymoreMine", 2, {}), ("LandMineTrap", 2, {}), ("M67Grenade", 3, {}), ("RGD5Grenade", 3, {}),
                   ("FlashGrenade", 2, {}), ("Grenade_ChemGas", 1, {}), ("M18SmokeGrenade_Green", 1, {}),
                   ("M18SmokeGrenade_Red", 1, {}), ("RDG2SmokeGrenade_Black", 1, {})]),
    # loose on the floor: a belt in cargo can't take attachments (vanilla MilitaryBelt)
    {"type": "MilitaryBelt", "attachments": [{"type": "Canteen", "liquid": "Water", "quantity": 1},
                                             {"type": "NylonKnifeSheath", "attachments": [{"type": "CombatKnife"}]},
                                             {"type": "PlateCarrierHolster"}]},
]}
drops.insert(0, EQUIPMENT)

old["drops"] = drops
old["guns"] = kits
json.dump(old, open(json_path, "w"), indent=4, ensure_ascii=False)
open(json_path, "a").write("\n")
print(len(drops), "loads:", ", ".join(d["name"] for d in drops))
print(len(kits), "gun kits:", ", ".join(k["name"] for k in kits))
