# TODO

## Testing

- [x] When a shipping crate self-destructs with a vehicle in it, does that trigger
  BetterExplosions (our mod, DayZMissions `mods/BetterExplosions`)? The blast is
  `DamageSystem.ExplosionDamage(..., "Plastic_Explosive_Ammo", ...)` from the crate
  (`SC_Detonate`/`SC_Blast` in `SC_Airdrop.c`), and the vehicle is deleted 2 s later
  (`SC_Remove`), which may be too soon for anything BetterExplosions does to show.
  The VPS (roles.nasdara) runs both, with BetterExplosions_Expansion, so watch a drop go up
  there: a container drop (a car) and a pallet drop (M3S or M1025).
  Answered on the VPS, 2026-11-13: yes. A pallet's blast set its M3S off ("hit by
  Plastic_Explosive_Ammo: it goes up") and swapped it for a wreck; it also set off an M3S parked
  13 m away. Since 0.4 a container's walls don't shield the car inside either.
