// Shipping Crate test mission: vanilla Chernarus with Expansion Missions and Shipping Crate.
// Played normally: hold the delivery radio to order a drop where you stand. To save waiting,
// Expansion's airdrop planes (deliveries and supply signals) fly at TEST_PLANE_SPEED instead of
// 25 m/s: about 2 minutes in from the map edge, ~16 km away. Expansion's own airdrop missions
// keep their usual schedule; create $profile:sc_drop_now to start one now (it should be a plain
// Expansion airdrop: Shipping Crate leaves them alone).
//
// Started with -missiontest (tools/test.sh shippingcrate) it runs headless checks instead and
// logs [SCTest] lines: every class name in the config is real, the radio's knob steps
// right, and a real crate dropped from 60 m turns into a loaded container when it lands. Then
// it shuts the server down.
void main()
{
	Hive ce = CreateHive();
	if (ce)
		ce.InitOffline();
	GetGame().GetWorld().SetDate(2026, 9, 20, 12, 0);    // midday (22:30 to see the lights at night)
}

class CustomMission: MissionServer
{
	static const vector TEST_NEAR = "4950 0 2400";   // the fields west of Balota airfield
	vector m_Spot;
	ShippingCrate_Airdrop m_Crate;
	int m_Polls;

	override void OnInit()
	{
		super.OnInit();
		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Test, 5000, false);
	}

	static const float TEST_PLANE_SPEED = 150;      // m/s: about 2 minutes in from the map edge
	static const float TEST_DROPZONE_SPEED = 60;    // m/s over the drop zone
	// Expansion's default Chernarus airdrop missions (the ones without spaces in their names)
	static ref array<string> TEST_MISSIONS = {"Random_Balota", "Random_Berezino", "Random_Chernogorsk", "Random_Elektrozavodsk", "Random_Myshkino", "Random_NEAF", "Random_Novodmitrovsk", "Random_NWAF", "Random_Sosnovka", "Random_Zelenogorsk"};
	bool m_TestDropsStarted;

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		if (m_TestDropsStarted)
			return;
		m_TestDropsStarted = true;
		ExpansionAirdropSettings air = GetExpansionSettings().GetAirdrop();
		air.Speed = TEST_PLANE_SPEED;
		air.DropZoneSpeed = TEST_DROPZONE_SPEED;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckDropNow, 2000, true);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RecordFlights, 1000, true);
	}

	// Flight recorder: once a second, every player in a vehicle: where, how high above the
	// ground, how fast (and climbing), and its attitude (yaw/pitch/roll, degrees). After a
	// client freezes, the last [SCFlight] lines show what the vehicle was doing.
	void RecordFlights()
	{
		array<Man> men = new array<Man>();
		GetGame().GetPlayers(men);
		foreach (Man man : men)
		{
			Transport t = Transport.Cast(man.GetParent());
			if (!t)
				continue;
			vector p = t.GetPosition();
			float agl = p[1] - GetGame().SurfaceY(p[0], p[2]);
			vector vel = GetVelocity(t);
			vector ori = t.GetOrientation();
			vector av = dBodyGetAngularVelocity(t);
			Print(string.Format("[SCFlight] %1 at %2 alt %3 m (%4 above ground) speed %5 m/s climb %6 ypr %7 spin %8", t.GetType(), p, p[1], agl, vel.Length(), vel[1], ori, av));
		}
	}

	// a vehicle beside the first player, made the way Expansion's admin tools make one (no drop
	// involved): create $profile:sc_spawn holding its class name, e.g. ExpansionUh1h
	static const float SPAWN_CLEAR_RADIUS = 50;

	void CheckSpawn()
	{
		if (!FileExist("$profile:sc_spawn"))
			return;
		array<Man> men = new array<Man>();
		GetGame().GetPlayers(men);
		if (men.Count() == 0)
			return;
		string type = "ExpansionUh1h";
		FileHandle fh = OpenFile("$profile:sc_spawn", FileMode.READ);
		if (fh)
		{
			string line;
			if (FGets(fh, line) > 0)
			{
				line.Trim();
				if (line != "")
					type = line;
			}
			CloseFile(fh);
		}
		DeleteFile("$profile:sc_spawn");
		Man man = men[0];
		vector p = man.GetPosition() + man.GetDirection() * 15;
		p[1] = GetGame().SurfaceY(p[0], p[2]) + 0.5;

		// earlier test vehicles of this type left around the spot (they're saved, so they come
		// back after a restart): gone first, unless someone is in one
		array<Object> near = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		GetGame().GetObjectsAtPosition(p, SPAWN_CLEAR_RADIUS, near, proxies);
		int cleared = 0;
		foreach (Object o : near)
		{
			Transport old = Transport.Cast(o);
			if (!old || old.GetType() != type)
				continue;
			bool crewed = false;
			for (int seat = 0; seat < old.CrewSize(); seat++)
			{
				if (old.CrewMember(seat))
					crewed = true;
			}
			if (crewed)
				continue;
			GetGame().ObjectDelete(old);
			cleared++;
		}
		if (cleared > 0)
			Print("[SCTest] removed " + cleared + " old " + type + " near the spawn spot");
		EntityAI v = EntityAI.Cast(GetGame().CreateObjectEx(type, p, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS));
		if (!v)
		{
			Print("[SCTest] could not spawn " + type);
			return;
		}
		v.OnDebugSpawn();
		v.SetLifetimeMax(3888000);
		v.SetLifetime(3888000);
		Print("[SCTest] spawned " + type + " at " + p + " beside " + man.GetIdentity().GetName());
	}

	// everyone onto dry land on request: create $profile:sc_tp, optionally holding "x z"
	// (default: the fields north of Elektrozavodsk)
	void CheckTeleport()
	{
		if (!FileExist("$profile:sc_tp"))
			return;
		vector to = "10790 0 2365";
		FileHandle fh = OpenFile("$profile:sc_tp", FileMode.READ);
		if (fh)
		{
			string line;
			if (FGets(fh, line) > 0)
			{
				TStringArray parts = new TStringArray();
				line.Split(" ", parts);
				if (parts.Count() >= 2)
					to = Vector(parts[0].ToFloat(), 0, parts[1].ToFloat());
			}
			CloseFile(fh);
		}
		array<Man> men = new array<Man>();
		GetGame().GetPlayers(men);
		if (men.Count() == 0)
			return;   // keep the file until someone is in
		DeleteFile("$profile:sc_tp");
		foreach (Man man : men)
		{
			vector p = to;
			p[1] = GetGame().SurfaceY(p[0], p[2]) + 0.3;
			man.SetPosition(p);
			Print("[SCTest] moved " + man.GetIdentity().GetName() + " to " + p);
		}
	}

	// an Expansion airdrop mission on request: create $profile:sc_drop_now and one starts within 2 s
	void CheckDropNow()
	{
		CheckTeleport();
		CheckSpawn();
		if (!FileExist("$profile:sc_drop_now"))
			return;
		DeleteFile("$profile:sc_drop_now");
		Print("[SCTest] drop requested");
		StartTestAirdrop();
	}

	void StartTestAirdrop()
	{
		ExpansionMissionModule module = ExpansionMissionModule.s_Instance;
		if (!module)
		{
			Print("[SCTest] no Expansion mission module - no test airdrop");
			return;
		}
		// Expansion allows MaxMissions at once (1 in its file). Raise it just while the test drop
		// starts, then put it back: left raised, Expansion's own scheduler fills the free slots
		// with a new airdrop every 10 s.
		ExpansionMissionSettings missionSettings = GetExpansionSettings().GetMission();
		int maxMissions = missionSettings.MaxMissions;
		missionSettings.MaxMissions = module.GetNumberRunningMissions() + 1;

		// the first one that isn't already running, from a random start
		int start = Math.RandomInt(0, TEST_MISSIONS.Count());
		for (int i = 0; i < TEST_MISSIONS.Count(); i++)
		{
			string name = TEST_MISSIONS[(start + i) % TEST_MISSIONS.Count()];
			ExpansionMissionEventAirdrop airdrop = ExpansionMissionEventAirdrop.Cast(module.FindMission("ExpansionMissionEventAirdrop", name));
			if (!airdrop || airdrop.IsRunning())
				continue;
			airdrop.Speed = TEST_PLANE_SPEED;
			airdrop.DropZoneSpeed = TEST_DROPZONE_SPEED;
			bool started = module.StartMission(airdrop);
			missionSettings.MaxMissions = maxMissions;
			Print("[SCTest] test airdrop " + name + " started: " + started + " (" + module.GetNumberRunningMissions() + " running)");
			return;
		}
		missionSettings.MaxMissions = maxMissions;
		Print("[SCTest] every test airdrop mission is busy - skipping this one");
	}

	void CheckLeftovers(Object riding, Object parked, Object onIt, Object onIt2, Object beside)
	{
		Print("[SCTest] after a restart mid-fall: riding truck " + riding + " (want NULL), parked truck kept " + (parked != null) + " (want 1)");
		Print("[SCTest] after a restart: on the pallet " + onIt + " / " + onIt2 + " (want NULL), beside it " + (beside != null) + " (want 1)");
		if (parked)
			GetGame().ObjectDelete(parked);
		if (beside)
			GetGame().ObjectDelete(beside);
	}

	// Every vehicle load with its parts, and every gun kit in every boot: what won't attach,
	// load or fit is printed by SC_Spawn.Fill ("could not attach/load", "no room for").
	// fluids read back a moment after filling
	void CheckFuel(EntityAI vehicle, string name)
	{
		CarScript car = CarScript.Cast(vehicle);
		MotorbikeScript bike = MotorbikeScript.Cast(vehicle);
		if (car)
			Print("[SCTest] loads: " + name + " fuel " + car.GetFluidFraction(CarFluid.FUEL) + " oil " + car.GetFluidFraction(CarFluid.OIL));
		else if (bike)
			Print("[SCTest] loads: " + name + " fuel " + bike.GetFluidFraction(MotorbikeFluid.FUEL));
		GetGame().ObjectDelete(vehicle);
	}

	void LoadsTest()
	{
		SC_Config cfg = SC_Settings.Get();
		int v = 0;
		foreach (SC_Drop drop : cfg.drops)
		{
			if (!drop.vehicle)
				continue;
			string type = SC_Settings.Choose(drop.vehicle.type);
			vector at = TEST_NEAR + Vector(-200 - 15 * v, 0, -200);
			at[1] = GetGame().SurfaceY(at[0], at[2]) + 1;
			v++;
			EntityAI vehicle = EntityAI.Cast(GetGame().CreateObjectEx(type, at, ECE_SETUP | ECE_CREATEPHYSICS | ECE_PLACE_ON_SURFACE));
			if (!vehicle)
			{
				Print("[SCTest] loads: could not create " + type);
				continue;
			}
			SC_Spawn.Fill(vehicle, drop.vehicle, true);
			int wanted = 0;
			foreach (SC_Item a : drop.vehicle.attachments)
				wanted += SC_Settings.Count(a);
			CarScript car = CarScript.Cast(vehicle);
			string fuel = "not a car";
			MotorbikeScript bike = MotorbikeScript.Cast(vehicle);
			if (car)
				fuel = "fuel " + car.GetFluidFraction(CarFluid.FUEL);
			else if (bike)
				fuel = "bike fuel " + bike.GetFluidFraction(MotorbikeFluid.FUEL);
			Print("[SCTest] loads: " + drop.name + " (" + type + ", " + GetGame().ConfigGetTextOut("CfgVehicles " + type + " displayName") + ") parts " + vehicle.GetInventory().AttachmentCount() + " of " + wanted + ", " + fuel + ", boot " + SC_Spawn.HasBoot(vehicle) + ", ruined " + vehicle.IsRuined() + ", size " + SC_Pallet.Size(type) + ", fits container " + SC_Pallet.FitsContainer(type));
			if (!SC_Spawn.HasBoot(vehicle))
			{
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckFuel, 1000, false, vehicle, drop.name);
				continue;
			}
			int kits = 0;
			foreach (SC_Gun gun : cfg.guns)
			{
				SC_Item kit = new SC_Item();
				kit.cargo = gun.cargo;
				int before = vehicle.GetInventory().GetCargo().GetItemCount();
				SC_Spawn.Fill(vehicle, kit, true);
				int stowed = vehicle.GetInventory().GetCargo().GetItemCount() - before;
				int expected = 0;
				foreach (SC_Item c : gun.cargo)
					expected += SC_Settings.Count(c);
				if (stowed != expected)
					Print("[SCTest] loads: " + drop.name + " boot took " + stowed + " of " + expected + " from the " + gun.name + " kit");
				kits++;
				// empty it again for the next kit (the boot contents from the load stay)
				CargoBase cargo = vehicle.GetInventory().GetCargo();
				for (int i = cargo.GetItemCount() - 1; i >= before; i--)
					GetGame().ObjectDelete(cargo.GetItem(i));
			}
			Print("[SCTest] loads: " + drop.name + " tried " + kits + " gun kits");
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckFuel, 1000, false, vehicle, drop.name);
		}
	}

	void Test()
	{
		LoadsTest();
		SC_Config cfg = SC_Settings.Get();
		Print("[SCTest] " + cfg.drops.Count() + " loads, enabled " + cfg.enabled + ", file exists: " + FileExist(SC_Settings.FILE));

		int unknown = 0;
		foreach (SC_Drop drop : cfg.drops)
		{
			if (drop.vehicle)
				unknown += CheckNames(drop.vehicle);
			foreach (SC_Item item : drop.items)
				unknown += CheckNames(item);
		}
		foreach (SC_Gun gunKit : cfg.guns)
		{
			foreach (SC_Item gunItem : gunKit.cargo)
				unknown += CheckNames(gunItem);
		}
		for (int c = 1; c <= SC_Settings.COLOURS.Count(); c++)
		{
			if (!GetGame().ConfigIsExisting("CfgVehicles " + SC_Settings.ContainerType(c)))
			{
				Print("[SCTest] unknown container " + SC_Settings.ContainerType(c));
				unknown++;
			}
		}
		Print("[SCTest] unknown class names: " + unknown);

		// a drop left standing by a restart: what's on it goes, what's beside it stays
		vector lf[4];
		lf[0] = "1 0 0";
		lf[1] = "0 1 0";
		lf[2] = "0 0 1";
		lf[3] = TEST_NEAR + "30 0 30";
		lf[3][1] = GetGame().SurfaceY(lf[3][0], lf[3][2]);
		SC_Leftovers.Remember("test pallet", lf, "-1.5 0 -5", "1.5 3 5");
		Object onIt = GetGame().CreateObjectEx("Barrel_Green", lf[3] + "0 0.5 2", ECE_PLACE_ON_SURFACE);
		Object onIt2 = GetGame().CreateObjectEx("SeaChest", lf[3] + "0.5 0.3 -3", ECE_PLACE_ON_SURFACE);
		Object beside = GetGame().CreateObjectEx("Barrel_Blue", lf[3] + "4 0.5 0", ECE_PLACE_ON_SURFACE);
		// ... and one whose pallet was still falling: its truck, up in the air, goes; the same
		// kind of truck parked nearby (not ours) stays
		vector dropPoint = TEST_NEAR + "60 0 60";
		dropPoint[1] = GetGame().SurfaceY(dropPoint[0], dropPoint[2]);
		EntityAI riding = EntityAI.Cast(GetGame().CreateObjectEx("Truck_01_Covered", dropPoint + "5 300 5", ECE_CREATEPHYSICS));
		EntityAI parked = EntityAI.Cast(GetGame().CreateObjectEx("Truck_01_Covered", dropPoint + "15 1 0", ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS));
		if (riding)
		{
			dBodyEnableGravity(riding, false);
			dBodyActive(riding, ActiveState.INACTIVE);
			int r1;
			int r2;
			int r3;
			int r4;
			riding.GetPersistentID(r1, r2, r3, r4);
			int p1;
			int p2;
			int p3;
			int p4;
			parked.GetPersistentID(p1, p2, p3, p4);
			Print("[SCTest] persistent IDs: riding " + r1 + " " + r2 + " " + r3 + " " + r4 + ", parked " + p1 + " " + p2 + " " + p3 + " " + p4);
			SC_Leftovers.RememberRide("test truck on a pallet", dropPoint, riding);
		}
		SC_Leftovers.ClearAll();
		// deletions happen at the end of the frame: look a second later
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckLeftovers, 1000, false, riding, parked, onIt, onIt2, beside);

		// the delivery radio: its knob steps forward and back through the vehicles, with "any" between the last and the first
		SC_DropRadio blank = SC_DropRadio.Cast(GetGame().CreateObjectEx("SC_DropRadio", TEST_NEAR + "0 1 5", ECE_SETUP));
		SC_DropRadio tuned = SC_DropRadio.Cast(GetGame().CreateObjectEx("SC_DropRadio", TEST_NEAR + "1 1 5", ECE_SETUP));
		if (!blank || !tuned)
			Print("[SCTest] radio: could not create SC_DropRadio");
		else
		{
			tuned.SC_TuneVehicle();
			tuned.SC_TuneVehicle();
			string forward = tuned.m_SC_TunedVehicle;
			tuned.SC_TuneVehicle(-1);
			string back = tuned.m_SC_TunedVehicle;
			tuned.SC_TuneVehicle(-1);
			string toAny = tuned.m_SC_TunedVehicle;
			tuned.SC_TuneVehicle(-1);
			string wrapped = tuned.m_SC_TunedVehicle;
			tuned.SC_TuneVehicle(1);
			tuned.SC_TuneVehicle(1);
			string pickedDrop = "nothing";
			if (tuned.SC_TunedDrop())
				pickedDrop = tuned.SC_TunedDrop().name;
			Print("[SCTest] radio: two forward " + forward + ", one back " + back + ", back again \"" + toAny + "\" (want \"\" = any), back once more " + wrapped + " (want the last); untuned is " + blank.SC_TunedDrop() + "; the tuned one: " + pickedDrop);
			GetGame().ObjectDelete(blank);
			GetGame().ObjectDelete(tuned);
		}

		vector near = TEST_NEAR;
		near[1] = GetGame().SurfaceY(near[0], near[2]);
		m_Spot = SC_Spot.FlatGround(near, cfg.searchRadius, cfg.maxSlope);
		Print("[SCTest] flat ground " + vector.Distance(m_Spot, near) + " m from the test point, at " + m_Spot);

		// a real airdrop crate, 60 m up: it falls, lands, and should become the container
		vector high = m_Spot;
		high[1] = high[1] + 60;
		m_Crate = ShippingCrate_Airdrop.Cast(GetGame().CreateObjectEx("ShippingCrate_Airdrop", high, ECE_CREATEPHYSICS | ECE_AIRBORNE | ECE_NOLIFETIME));
		if (!m_Crate)
		{
			Print("[SCTest] could not create the crate");
			GetGame().RequestExit(0);
			return;
		}
		m_Crate.SetPosition(high);
		m_Crate.SC_Setup(SC_Settings.Pick(), SC_Settings.MixedGuns());   // the guns knob on "any"
		m_Crate.Expansion_InitAirdrop(new array<ref ExpansionLoot>(), new TStringArray(), 0, 0, 4.5, 0);
		Print("[SCTest] crate dropped from " + high);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(WaitForLanding, 1000, false);
	}

	void WaitForLanding()
	{
		m_Polls++;
		if (m_Crate && !m_Crate.SC_HasLanded() && m_Polls <= 90)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(WaitForLanding, 1000, false);
			return;
		}
		Print("[SCTest] container in place after " + m_Polls + " s; crate parked at " + m_Crate.GetPosition());
		// let the vehicle settle on the container floor before looking at it
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Inspect, 8000, false);
	}

	void Inspect()
	{
		array<Object> objects = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		GetGame().GetObjectsAtPosition(m_Spot, 15, objects, proxies);
		Building box;
		CarScript car;
		foreach (Object obj : objects)
		{
			if (obj.IsInherited(ContainerLockedBase))
				Class.CastTo(box, obj);
			if (obj.IsInherited(CarScript))
				Class.CastTo(car, obj);
		}
		Print("[SCTest] container: " + box);
		if (box)
		{
			string doors = "";
			for (int d = 0; d < box.GetDoorCount(); d++)
				doors += " " + d + ":locked=" + box.IsDoorLocked(d) + ",soundpos=" + box.WorldToModel(box.GetDoorSoundPos(d));
			Print("[SCTest] doors" + doors);
		}
		if (!car)
		{
			Print("[SCTest] no vehicle near the spot");
		}
		else
		{
			vector rel = car.GetPosition();
			if (box)
				rel = box.WorldToModel(car.GetPosition());
			vector up = car.GetDirectionUp();
			vector fwd = car.GetDirection();
			if (box)
				fwd = box.VectorToLocal(fwd);
			Print("[SCTest] vehicle " + car.GetType() + " at " + rel + " in the container, up " + up + ", forward " + fwd + ", ruined " + car.IsDamageDestroyed());
			Print("[SCTest] fuel " + car.GetFluidFraction(CarFluid.FUEL) + " oil " + car.GetFluidFraction(CarFluid.OIL) + " coolant " + car.GetFluidFraction(CarFluid.COOLANT) + " brake " + car.GetFluidFraction(CarFluid.BRAKE));
			string parts = "";
			for (int pi = 0; pi < car.GetInventory().AttachmentCount(); pi++)
				parts += " " + car.GetInventory().GetAttachmentFromIndex(pi).GetType();
			Print("[SCTest] parts fitted " + car.GetInventory().AttachmentCount() + " of " + car.GetInventory().GetAttachmentSlotsCount() + ":" + parts);

			CargoBase cargo = car.GetInventory().GetCargo();
			int n = 0;
			if (cargo)
				n = cargo.GetItemCount();
			Print("[SCTest] trunk holds " + n + " things");
			for (int i = 0; i < n; i++)
			{
				EntityAI thing = cargo.GetItem(i);
				string info = thing.GetType();
				Weapon_Base w;
				if (Class.CastTo(w, thing))
				{
					info += " rounds " + w.GetTotalCartridgeCount(0) + " attachments";
					for (int a = 0; a < w.GetInventory().AttachmentCount(); a++)
						info += " " + w.GetInventory().GetAttachmentFromIndex(a).GetType();
				}
				Magazine mag;
				if (Class.CastTo(mag, thing))
					info += " rounds " + mag.GetAmmoCount();
				ItemBase ib;
				if (Class.CastTo(ib, thing) && ib.IsLiquidContainer())
					info += " liquid " + ib.GetLiquidType() + " quantity " + ib.GetQuantityNormalized();
				Print("[SCTest]   " + info);
			}
		}

		int loose = 0;
		foreach (Object o : objects)
		{
			if (o.IsInherited(ItemBase) && !EntityAI.Cast(o).GetHierarchyParent())
			{
				vector lp = o.GetPosition();
				if (box)
					lp = box.WorldToModel(lp);
				Print("[SCTest] on the floor: " + o.GetType() + " at " + lp);
				loose++;
			}
		}
		Print("[SCTest] " + loose + " loose items");

		// the self-destruct, now rather than 10 minutes after landing: the bear warns, the roof
		// blows, and the container and everything in it (the Ada, the hand truck) are removed
		m_Crate.SC_Warn();
		m_Crate.SC_Detonate();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckBlown, 1000, false, "container", car);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AfterEnd, 4000, false, box, car);
	}

	// a second after the blast, before the removal: what was in or on the carrier is destroyed
	// (or, with Better Explosions, already gone up and swapped for a wreck)
	void CheckBlown(string carrier, EntityAI vehicle)
	{
		if (!vehicle)
			Print("[SCTest] " + carrier + " blast: the vehicle went up (swapped for a wreck)");
		else
			Print("[SCTest] " + carrier + " blast: " + vehicle.GetType() + " ruined " + vehicle.IsRuined());
	}

	void AfterEnd(Building box, CarScript car)
	{
		Print("[SCTest] after the self-destruct: crate " + m_Crate + ", container " + box + ", vehicle " + car);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(PalletTest, 1000, false);
	}

	// A load too big for the container: an M3S truck, which should come down on a pallet with
	// a barrel and a sea chest on the free end, then blow up and go like the container.
	ShippingCrate_Airdrop m_Pallet;
	vector m_PalletSpot;
	int m_PalletPolls;

	void PalletTest()
	{
		// the config's own truck load if there is one, else one made here
		foreach (SC_Drop configured : SC_Settings.Get().drops)
		{
			if (configured.name == "M3S Covered")
			{
				StartPallet(configured);
				return;
			}
		}
		SC_Drop truck = new SC_Drop();
		truck.name = "pallet test";
		truck.vehicle = new SC_Item();
		truck.vehicle.type = "Truck_01_Covered";
		truck.vehicle.fluids = true;
		array<string> parts = {"Truck_01_Wheel", "Truck_01_Wheel", "Truck_01_WheelDouble", "Truck_01_WheelDouble", "Truck_01_WheelDouble", "Truck_01_WheelDouble", "TruckBattery"};
		foreach (string part : parts)
		{
			SC_Item a = new SC_Item();
			a.type = part;
			truck.vehicle.attachments.Insert(a);
		}
		array<string> loose = {"Barrel_Blue", "SeaChest"};
		foreach (string l : loose)
		{
			SC_Item it = new SC_Item();
			it.type = l;
			truck.items.Insert(it);
		}

		StartPallet(truck);
	}

	void StartPallet(SC_Drop truck)
	{
		m_PalletSpot = SC_Spot.FlatGround(m_Spot + "60 0 0", 150, 8);
		vector high = m_PalletSpot;
		high[1] = high[1] + 60;
		m_Pallet = ShippingCrate_Airdrop.Cast(GetGame().CreateObjectEx("ShippingCrate_Airdrop", high, ECE_CREATEPHYSICS | ECE_AIRBORNE | ECE_NOLIFETIME));
		m_Pallet.SetPosition(high);
		m_Pallet.SC_Setup(truck, SC_Settings.PickGun());
		m_Pallet.Expansion_InitAirdrop(new array<ref ExpansionLoot>(), new TStringArray(), 0, 0, 4.5, 0);
		Print("[SCTest] pallet: truck crate dropped from " + high + ", carrier " + m_Pallet.m_SC_Carrier + " (1 = pallet)");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(WaitForPallet, 1000, false);
	}

	void WaitForPallet()
	{
		m_PalletPolls++;
		if (m_Pallet && !m_Pallet.SC_HasLanded() && m_PalletPolls <= 90)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(WaitForPallet, 1000, false);
			return;
		}
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(InspectPallet, 8000, false);
	}

	void InspectPallet()
	{
		array<Object> objects = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		GetGame().GetObjectsAtPosition(m_PalletSpot, 15, objects, proxies);
		int tiles = 0;
		CarScript truck;
		string items = "";
		foreach (Object o : objects)
		{
			if (o.GetType() == SC_Pallet.TILE)
				tiles++;
			if (o.IsInherited(CarScript))
				Class.CastTo(truck, o);
			if (o.IsInherited(ItemBase) && !EntityAI.Cast(o).GetHierarchyParent())
				items += " " + o.GetType() + " (" + (o.GetPosition()[1] - GetGame().SurfaceY(o.GetPosition()[0], o.GetPosition()[2])) + " m up)";
			if (o.GetType() == "SeaChest" && EntityAI.Cast(o).GetInventory().GetCargo())
			{
				CargoBase chest = EntityAI.Cast(o).GetInventory().GetCargo();
				string inChest = "";
				for (int chestI = 0; chestI < chest.GetItemCount(); chestI++)
					inChest += " " + chest.GetItem(chestI).GetType();
				Print("[SCTest] pallet: sea chest holds " + chest.GetItemCount() + ":" + inChest);
			}
		}
		Print("[SCTest] pallet: " + tiles + " pallets, loose items:" + items);
		if (truck)
		{
			string fitted = "";
			for (int pi = 0; pi < truck.GetInventory().AttachmentCount(); pi++)
				fitted += " " + truck.GetInventory().GetAttachmentFromIndex(pi).GetType();
			string carried = "";
			CargoBase cargo = truck.GetInventory().GetCargo();
			if (cargo)
			{
				for (int ci = 0; ci < cargo.GetItemCount(); ci++)
					carried += " " + cargo.GetItem(ci).GetType();
			}
			Print("[SCTest] pallet: parts" + fitted + "; cargo" + carried + "; " + truck.GetInventory().GetAttachmentSlotsCount() + " slots");
		}
		if (truck)
			Print("[SCTest] pallet: " + truck.GetType() + " bottom " + (truck.GetPosition()[1] - GetGame().SurfaceY(truck.GetPosition()[0], truck.GetPosition()[2])) + " m up, up " + truck.GetDirectionUp() + ", parts " + truck.GetInventory().AttachmentCount() + ", ruined " + truck.IsDamageDestroyed());
		else
			Print("[SCTest] pallet: no truck");
		m_Pallet.SC_Warn();
		m_Pallet.SC_Detonate();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckBlown, 1000, false, "pallet", truck);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AfterPallet, 4000, false, truck);
	}

	void AfterPallet(CarScript truck)
	{
		array<Object> objects = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		GetGame().GetObjectsAtPosition(m_PalletSpot, 15, objects, proxies);
		int tiles = 0;
		foreach (Object o : objects)
		{
			if (o.GetType() == SC_Pallet.TILE)
				tiles++;
		}
		Print("[SCTest] pallet after the self-destruct: crate " + m_Pallet + ", truck " + truck + ", pallets left " + tiles);
		GetGame().RequestExit(0);
	}

	// counts class names in spec (and on and in it) the game doesn't know
	int CheckNames(SC_Item spec)
	{
		int unknown = 0;
		array<string> options = new array<string>();
		spec.type.Split("|", options);
		if (spec.magazine != "")
			options.Insert(spec.magazine);
		foreach (string o : options)
		{
			o.Trim();
			if (!GetGame().ConfigIsExisting("CfgVehicles " + o) && !GetGame().ConfigIsExisting("CfgWeapons " + o) && !GetGame().ConfigIsExisting("CfgMagazines " + o))
			{
				Print("[SCTest] unknown class " + o);
				unknown++;
			}
		}
		foreach (SC_Item a : spec.attachments)
			unknown += CheckNames(a);
		foreach (SC_Item c : spec.cargo)
			unknown += CheckNames(c);
		return unknown;
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
