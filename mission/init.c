// Shipping Crate test mission: vanilla Chernarus with Expansion Missions and Shipping Crate.
// Played normally, Expansion's airdrops land as shipping containers on the players. To save
// waiting, an airdrop starts as soon as the first player logs in and then every
// TEST_INTERVAL_MS, each one a different airdrop mission (Expansion's one-at-a-time limit is
// lifted for the moment it starts), with the plane at TEST_PLANE_SPEED instead of 25 m/s: it comes in from the
// map edge, ~16 km away. Earlier containers stay until Expansion ends their missions (20 min,
// and nobody within 1100 m).
//
// Started with -missiontest (tools/test.sh shippingcrate) it runs headless checks instead and
// logs [SCTest] lines: every class name in the config is real, the grouping finds the right
// centre, and a real crate dropped from 60 m turns into a loaded container when it lands. Then
// it shuts the server down.
void main()
{
	Hive ce = CreateHive();
	if (ce)
		ce.InitOffline();
	GetGame().GetWorld().SetDate(2026, 9, 20, 22, 30);   // night, to see the lights
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
	static const int TEST_INTERVAL_MS = 600000;     // 10 minutes
	// Expansion's default Chernarus airdrop missions (the ones without spaces in their names)
	static ref array<string> TEST_MISSIONS = {"Random_Balota", "Random_Berezino", "Random_Chernogorsk", "Random_Elektrozavodsk", "Random_Myshkino", "Random_NEAF", "Random_Novodmitrovsk", "Random_NWAF", "Random_Sosnovka", "Random_Zelenogorsk"};
	bool m_TestDropsStarted;

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		if (m_TestDropsStarted)
			return;
		m_TestDropsStarted = true;
		// a few seconds, so the player is in the world when the group is worked out
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StartTestAirdrop, 5000, false);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StartTestAirdrop, TEST_INTERVAL_MS, true);
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

		// any airdrop mission will do, Shipping Crate moves it onto the players anyway; the
		// first one that isn't already running, from a random start
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

	void Test()
	{
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
		for (int c = 1; c <= SC_Settings.COLOURS.Count(); c++)
		{
			if (!GetGame().ConfigIsExisting("CfgVehicles " + SC_Settings.ContainerType(c)))
			{
				Print("[SCTest] unknown container " + SC_Settings.ContainerType(c));
				unknown++;
			}
		}
		Print("[SCTest] unknown class names: " + unknown);

		// two players together in Chernogorsk, one alone at NWAF: the group of two wins
		array<vector> players = {"6500 0 2500", "6560 0 2440", "4800 0 9800"};
		vector centre;
		int size;
		SC_Spot.Cluster(players, 1000, centre, size);
		Print("[SCTest] group of " + size + " centred at " + centre + " (expected 2 at 6530 2470), nearest town " + SC_Spot.NearestTown(centre));

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
		m_Crate.m_SC_Drop = SC_Settings.Pick();
		m_Crate.SC_SetColour(SC_Settings.PickColour());
		m_Crate.Expansion_InitAirdrop(new array<ref ExpansionLoot>(), new TStringArray(), 0, 0, 4.5, 0);
		Print("[SCTest] crate dropped from " + high);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(WaitForLanding, 1000, false);
	}

	void WaitForLanding()
	{
		m_Polls++;
		if (m_Crate && !m_Crate.m_SC_Box && m_Polls <= 90)
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
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AfterEnd, 4000, false, box, car);
	}

	void AfterEnd(Building box, CarScript car)
	{
		Print("[SCTest] after the self-destruct: crate " + m_Crate + ", container " + box + ", vehicle " + car);
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
