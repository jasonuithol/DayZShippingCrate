// The hooks into Expansion Missions' airdrops.
//
// 1. When an airdrop mission starts, its drop location is moved to the centre of the biggest
//    group of players (on flat, clear ground) and its infected count set to zero.
// 2. Its plane drops a ShippingCrate_Airdrop instead of the usual crate: Expansion's own crate,
//    falling the usual way, but with no loot and no infected. Clients don't draw it: they draw a
//    shipping container hanging under two parachutes in its place.
// 3. When it comes to rest the server puts the real container there and loads it from the
//    config. The crate itself is parked out of sight under the container floor, so Expansion's
//    own rule still ends the mission (time up and nobody within 1100 m); when Expansion deletes
//    the crate, the container goes with it. The vehicle and everything else stays.
//
// Player-called airdrops (supply signals) are left alone: only missions are hijacked.
//
// ExpansionMissionEventAirdrop itself can't be modded (it hands itself to a JSON template, which
// breaks with a modded type), so the hook is on its base class.

modded class ExpansionMissionEventBase
{
	// [NonSerialized()]: Expansion saves missions as JSON files
	[NonSerialized()]
	bool m_SC_Active;
	[NonSerialized()]
	SC_Drop m_SC_Drop;
	[NonSerialized()]
	ref ExpansionAirdropLocation m_SC_SavedLocation;
	[NonSerialized()]
	int m_SC_SavedInfectedCount;

	override void Start()
	{
		ExpansionMissionEventAirdrop airdrop;
		m_SC_Active = false;
		if (g_Game.IsServer() && Class.CastTo(airdrop, this))
			SC_Prepare(airdrop);

		super.Start();

		if (m_SC_Active)
		{
			if (airdrop.m_Plane)
			{
				airdrop.m_Plane.m_SC_Mission = this;
			}
			else
			{
				SC_Restore(airdrop);
				m_SC_Active = false;
			}
		}
	}

	override void End()
	{
		super.End();

		ExpansionMissionEventAirdrop airdrop;
		if (m_SC_Active && Class.CastTo(airdrop, this))
			SC_Restore(airdrop);
		m_SC_Active = false;
	}

	protected void SC_Prepare(ExpansionMissionEventAirdrop airdrop)
	{
		SC_Config cfg = SC_Settings.Get();
		if (!cfg.enabled)
			return;

		SC_Drop drop = SC_Settings.Pick();
		if (!drop)
		{
			Print("[ShippingCrate] no loads configured - a normal Expansion airdrop");
			return;
		}

		vector centre;
		int size;
		if (!SC_Spot.Cluster(SC_Spot.PlayerPositions(), cfg.clusterRadius, centre, size))
		{
			Print("[ShippingCrate] nobody is online - a normal Expansion airdrop");
			return;
		}
		vector spot = SC_Spot.FlatGround(centre, cfg.searchRadius, cfg.maxSlope);
		string town = SC_Spot.NearestTown(spot);

		m_SC_SavedLocation = airdrop.DropLocation;
		m_SC_SavedInfectedCount = airdrop.InfectedCount;
		airdrop.DropLocation = new ExpansionAirdropLocation(spot[0], spot[2], cfg.dropRadius, town);
		airdrop.InfectedCount = 0;
		m_SC_Drop = drop;
		m_SC_Active = true;
		Print("[ShippingCrate] " + MissionName + ": shipping container (" + drop.name + ") for a group of " + size + " near " + town + " at " + spot);
	}

	// put the mission's own location back, so the next run starts from the file's values
	protected void SC_Restore(ExpansionMissionEventAirdrop airdrop)
	{
		if (m_SC_SavedLocation)
			airdrop.DropLocation = m_SC_SavedLocation;
		airdrop.InfectedCount = m_SC_SavedInfectedCount;
		m_SC_SavedLocation = null;
	}
}

modded class ExpansionAirdropPlaneBase
{
	ExpansionMissionEventBase m_SC_Mission;

	// client: navigation lights on the wingtips, on every airdrop plane
	protected ScriptedLightBase m_SC_NavLeft;
	protected ScriptedLightBase m_SC_NavRight;
	protected ScriptedLightBase m_SC_NavTail;
	protected Object m_SC_MarkLeft;
	protected Object m_SC_MarkRight;
	protected Object m_SC_MarkTail;
	protected vector m_SC_LeftPos;
	protected vector m_SC_RightPos;
	protected vector m_SC_TailPos;
	protected bool m_SC_NavFollowing;
	protected float m_SC_NavTime;

	override void EEInit()
	{
		super.EEInit();
		if (!g_Game.IsDedicatedServer())
			SC_AddNavLights();
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		SC_StopNavFollow();
		SC_Lights.Destroy(m_SC_NavLeft);
		SC_Lights.Destroy(m_SC_NavRight);
		SC_Lights.Destroy(m_SC_NavTail);
		m_SC_NavLeft = null;
		m_SC_NavRight = null;
		m_SC_NavTail = null;
		if (m_SC_MarkLeft)
			g_Game.ObjectDelete(m_SC_MarkLeft);
		if (m_SC_MarkRight)
			g_Game.ObjectDelete(m_SC_MarkRight);
		if (m_SC_MarkTail)
			g_Game.ObjectDelete(m_SC_MarkTail);
		m_SC_MarkLeft = null;
		m_SC_MarkRight = null;
		m_SC_MarkTail = null;
	}

	// only unhooks (see ShippingCrate_Airdrop's destructor)
	void ~ExpansionAirdropPlaneBase()
	{
		SC_StopNavFollow();
	}

	protected void SC_StopNavFollow()
	{
		if (m_SC_NavFollowing && g_Game && g_Game.GetUpdateQueue(CALL_CATEGORY_GUI))
			g_Game.GetUpdateQueue(CALL_CATEGORY_GUI).Remove(SC_NavFollow);
		m_SC_NavFollowing = false;
	}

	// the markers ride under the wingtips; the chemlight script may switch them back to their
	// "off" material, so the glow is put back every couple of seconds
	void SC_NavFollow(float dt)
	{
		m_SC_NavTime += dt;
		bool refresh = m_SC_NavTime >= 2;
		if (refresh)
			m_SC_NavTime = 0;
		SC_PlaceMarker(m_SC_MarkLeft, m_SC_LeftPos, SC_Lights.NAV_RED, "#(argb,8,8,3)color(1,0,0,1,CO)", refresh);
		SC_PlaceMarker(m_SC_MarkRight, m_SC_RightPos, SC_Lights.NAV_GREEN, "#(argb,8,8,3)color(0,1,0.2,1,CO)", refresh);
		SC_PlaceMarker(m_SC_MarkTail, m_SC_TailPos, SC_Lights.NAV_WHITE, "#(argb,8,8,3)color(1,1,1,1,CO)", refresh);
	}

	protected void SC_PlaceMarker(Object marker, vector modelPos, string material, string texture, bool refresh)
	{
		if (!marker)
			return;
		vector mat[4];
		GetTransform(mat);
		mat[3] = ModelToWorld(modelPos);
		marker.SetTransform(mat);
		marker.SetScale(SC_Lights.NAV_MARKER_SCALE);
		if (refresh)
			SC_Lights.Dress(marker, material, texture);
	}

	protected void SC_AddNavLights()
	{
		vector mm[2];
		ClippingInfo(mm);
		// Expansion flies building-type planes (the C130 wreck model) tail first in model
		// space: their forward is -z. Right is forward turned 90 degrees clockwise, so it's
		// +x when flying +z and -x when flying -z.
		float rightX = mm[1][0];
		float leftX = mm[0][0];
		if (IsBuilding())
		{
			rightX = mm[0][0];
			leftX = mm[1][0];
		}
		SC_LogMemoryPoints();
		vector leftPos = SC_UnderWing(leftX, mm);
		vector rightPos = SC_UnderWing(rightX, mm);

		m_SC_NavLeft = SC_Lights.Create(SC_NavLight, ModelToWorld(leftPos));
		m_SC_NavRight = SC_Lights.Create(SC_NavLight, ModelToWorld(rightPos));
		if (m_SC_NavLeft)
		{
			m_SC_NavLeft.SetAmbientColor(1.0, 0.05, 0.05);
			m_SC_NavLeft.SetDiffuseColor(1.0, 0.05, 0.05);
			m_SC_NavLeft.AttachOnObject(this, leftPos);
		}
		if (m_SC_NavRight)
		{
			m_SC_NavRight.SetAmbientColor(0.05, 1.0, 0.2);
			m_SC_NavRight.SetDiffuseColor(0.05, 1.0, 0.2);
			m_SC_NavRight.AttachOnObject(this, rightPos);
		}
		m_SC_LeftPos = leftPos;
		m_SC_RightPos = rightPos;
		m_SC_MarkLeft = SC_Lights.CreateNavMarker("SC_Prop_GlowRed", SC_Lights.NAV_RED, "#(argb,8,8,3)color(1,0,0,1,CO)", ModelToWorld(leftPos));
		m_SC_MarkRight = SC_Lights.CreateNavMarker("SC_Prop_GlowGreen", SC_Lights.NAV_GREEN, "#(argb,8,8,3)color(0,1,0.2,1,CO)", ModelToWorld(rightPos));
		// the tail light: white, just behind the rudder hinge (tail is +z when flying -z)
		m_SC_TailPos = SC_TailPoint(mm);
		m_SC_NavTail = SC_Lights.Create(SC_NavLight, ModelToWorld(m_SC_TailPos));
		if (m_SC_NavTail)
		{
			m_SC_NavTail.SetAmbientColor(1, 1, 1);
			m_SC_NavTail.SetDiffuseColor(1, 1, 1);
			m_SC_NavTail.AttachOnObject(this, m_SC_TailPos);
		}
		m_SC_MarkTail = SC_Lights.CreateNavMarker("SC_Prop_GlowWhite", SC_Lights.NAV_WHITE, "#(argb,8,8,3)color(1,1,1,1,CO)", ModelToWorld(m_SC_TailPos));

		m_SC_NavFollowing = true;
		g_Game.GetUpdateQueue(CALL_CATEGORY_GUI).Insert(SC_NavFollow);

		Print("[ShippingCrate] " + GetType() + " (" + ConfigGetString("model") + ") size " + (mm[1] - mm[0]) + ": nav lights red " + m_SC_MarkLeft + " at " + leftPos + ", green " + m_SC_MarkRight + " at " + rightPos + ", white " + m_SC_MarkTail + " at " + m_SC_TailPos);
	}

	// Model-space point a little under the wing near tipX. Rays don't hit the plane (clients
	// create it locally, without collision) and its LODs can't be read here, but its memory
	// LOD has the flap and aileron hinge points: the one furthest out gives the wing's height
	// and fore-aft position (the wing is symmetric, so it does for both sides).
	protected vector SC_UnderWing(float tipX, vector mm[2])
	{
		float x = tipX * 0.92;   // in from the very tip
		vector fallback = Vector(x, (mm[0][1] + mm[1][1]) * 0.5, (mm[0][2] + mm[1][2]) * 0.5);

		LOD memory = GetLODByName(LOD.NAME_MEMORY);
		if (!memory)
			return fallback;
		array<Selection> selections = new array<Selection>();
		memory.GetSelections(selections);
		string best;
		vector bestPos;
		foreach (Selection sel : selections)
		{
			string name = sel.GetName();
			string lower = name;
			lower.ToLower();
			if (lower.IndexOf("aileron") < 0 && lower.IndexOf("flap") < 0)
				continue;
			vector p = GetMemoryPointPos(name);
			if (best == "" || Math.AbsFloat(p[0]) > Math.AbsFloat(bestPos[0]))
			{
				best = name;
				bestPos = p;
			}
		}
		if (best == "")
		{
			Print("[ShippingCrate] " + GetType() + ": no flap or aileron points, lights at mid height");
			return fallback;
		}
		Print("[ShippingCrate] " + GetType() + ": wing from memory point " + best + " at " + bestPos);
		return Vector(x, bestPos[1] - 0.6, bestPos[2]);
	}

	// model-space point at the very back: just behind the rudder hinge if the model has one,
	// else the end of the bounding box on the centreline
	protected vector SC_TailPoint(vector mm[2])
	{
		float back = mm[0][2];        // tail is -z when flying +z ...
		float sign = -1;
		if (IsBuilding())
		{
			back = mm[1][2];          // ... and +z when flying -z
			sign = 1;
		}
		vector p = Vector(0, (mm[0][1] + mm[1][1]) * 0.5, back - sign * 0.5);
		if (MemoryPointExists("rudder_axis"))
		{
			vector rudder = GetMemoryPointPos("rudder_axis");
			p = Vector(0, rudder[1], rudder[2] + sign * 0.8);
			Print("[ShippingCrate] " + GetType() + ": tail light behind rudder_axis " + rudder);
		}
		return p;
	}

	// the model's memory points, for reference (a model may carry its own light positions)
	protected void SC_LogMemoryPoints()
	{
		LOD memory = GetLODByName(LOD.NAME_MEMORY);
		if (!memory)
			return;
		array<Selection> selections = new array<Selection>();
		memory.GetSelections(selections);
		string names = "";
		foreach (Selection sel : selections)
			names += " " + sel.GetName();
		Print("[ShippingCrate] " + GetType() + " memory points:" + names);
	}

#ifdef SERVER
	override ExpansionAirdropContainerBase Expansion_CreateDrop(string container)
	{
		if (!m_SC_Mission)
			return super.Expansion_CreateDrop(container);

		ExpansionAirdropContainerBase drop = super.Expansion_CreateDrop("ShippingCrate_Airdrop");
		ShippingCrate_Airdrop crate;
		if (Class.CastTo(crate, drop))
		{
			crate.m_SC_Mission = m_SC_Mission;
			crate.m_SC_Drop = m_SC_Mission.m_SC_Drop;
			crate.SC_SetColour(SC_Settings.PickColour());
		}
		return drop;
	}
#endif
}

class ShippingCrate_Airdrop: ExpansionAirdropContainerBase_Server
{
	int m_SC_Colour;   // 1-based index into SC_Settings.COLOURS, 0 until the server picks one

	// server
	ExpansionMissionEventBase m_SC_Mission;
	SC_Drop m_SC_Drop;
	Building m_SC_Box;
	protected bool m_SC_Waiting;
	protected bool m_SC_Delivered;
	protected int m_SC_Checks;

	static const float PARK_DEPTH = 3;   // the crate waits this far underground once the container is there

	// client: Expansion's crate stays hidden; in its place a local shipping container whose floor
	// is level with the crate's bottom, a parachute over each end, a spotlight underneath and a
	// rotating beacon at each end. Once it has landed only the beacons stay, on the real
	// container, until the mission ends and the crate goes.
	protected Object m_SC_Local;
	protected EntityAI m_SC_ChuteA;
	protected EntityAI m_SC_ChuteB;
	protected ScriptedLightBase m_SC_Spot;
	protected ScriptedLightBase m_SC_BeamA;
	protected ScriptedLightBase m_SC_BeamB;
	protected Object m_SC_PostA;
	protected Object m_SC_PostB;
	// the self-destruct: 0 landed (or falling), 1 the bear's warning, 2 blown
	int m_SC_Stage;

	// client: the roof decoration, a 4 x 2 grid of IEDs with a teddy bear sitting on top
	protected ref array<Object> m_SC_Charges = new array<Object>();
	protected Object m_SC_Bear;
	protected Object m_SC_EyeL;
	protected Object m_SC_EyeR;
	protected ScriptedLightBase m_SC_EyeLightL;
	protected ScriptedLightBase m_SC_EyeLightR;
	protected float m_SC_NextVoice;
	protected bool m_SC_LoggedDecor;

	static const string CHARGE = "SC_Prop_IED";       // config.cpp: the IED model, not an item
	static const string BEAR = "SC_Prop_Bear";        // config.cpp: vanilla's Bear_Beige model, not an item
	static const string EVACUATE_SOUND = "SC_Evacuate_SoundSet";
	static const float VOICE_EVERY = 3;
	static const string BLAST = "Plastic_Explosive_Ammo";
	protected Object m_SC_Landed;
	protected bool m_SC_Following;
	protected float m_SC_Time;
	protected float m_SC_NextSearch;

	void ShippingCrate_Airdrop()
	{
		RegisterNetSyncVariableInt("m_SC_Colour", 0, 8);
		RegisterNetSyncVariableInt("m_SC_Stage", 0, 2);
	}

	// Only unhooks: by the time the destructor runs (e.g. leaving the server) the local objects
	// may already be gone, and deleting them from here crashes the game.
	void ~ShippingCrate_Airdrop()
	{
		SC_StopFollowing();
	}

	override void EEInit()
	{
		super.EEInit();
		if (!g_Game.IsDedicatedServer())
		{
			m_SC_Following = true;
			g_Game.GetUpdateQueue(CALL_CATEGORY_GUI).Insert(SC_Follow);
		}
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (g_Game.IsDedicatedServer())
		{
			// Expansion deletes the crate when the mission ends: the container goes too
#ifdef SERVER
			SC_CancelTimers();
#endif
			if (m_SC_Box)
				g_Game.ObjectDelete(m_SC_Box);
			m_SC_Box = null;
			return;
		}
		SC_DeleteDecor();
		SC_StopFollowing();
		SC_DeleteLocals();
		SC_Lights.Destroy(m_SC_BeamA);
		SC_Lights.Destroy(m_SC_BeamB);
		m_SC_BeamA = null;
		m_SC_BeamB = null;
		if (m_SC_PostA)
			g_Game.ObjectDelete(m_SC_PostA);
		if (m_SC_PostB)
			g_Game.ObjectDelete(m_SC_PostB);
		m_SC_PostA = null;
		m_SC_PostB = null;
	}

#ifdef SERVER
	void SC_SetColour(int colour)
	{
		m_SC_Colour = colour;
		SetSynchDirty();
	}

	// no loot in the crate, no infected around it, whatever the mission or container settings say
	override void Expansion_InitAirdrop(array<ref ExpansionLoot> Loot, TStringArray infected, int ItemCount, int infectedCount, float fallSpeed = 4.5, float windImpact = 0.0)
	{
		super.Expansion_InitAirdrop(new array<ref ExpansionLoot>(), new TStringArray(), 0, 0, fallSpeed, windImpact);
	}

	// Expansion divides by dt while the crate falls; two updates in one server tick make it 0
	override void Expansion_OnUpdate(float dt)
	{
		if (dt <= 0)
			return;
		super.Expansion_OnUpdate(dt);
	}

	override void Expansion_DiscardParachute()
	{
		super.Expansion_DiscardParachute();

		if (!m_SC_Waiting)
		{
			m_SC_Waiting = true;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_WaitForRest, 250, false);
		}
	}

	// once the crate has stopped moving (or after 10 s), the container takes its place
	protected void SC_WaitForRest()
	{
		if (m_SC_Delivered)
			return;
		m_SC_Checks++;
		if (dBodyIsActive(this) && m_SC_Checks <= 40)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_WaitForRest, 250, false);
			return;
		}
		m_SC_Delivered = true;

		SC_Drop drop = m_SC_Drop;
		if (!drop && m_SC_Mission)
			drop = m_SC_Mission.m_SC_Drop;
		if (!drop)
			drop = SC_Settings.Pick();
		int colour = m_SC_Colour;
		if (colour == 0)
			colour = SC_Settings.PickColour();

		vector pos = GetPosition();
		float yaw = GetOrientation()[0];

		// out of the way first: the vehicle goes where the crate is
		Expansion_DisableUpdate();
		if (m_Expansion_ForceCollisionInit_Object)
			g_Game.ObjectDelete(m_Expansion_ForceCollisionInit_Object);   // Expansion's dead zombie
		m_Expansion_ForceCollisionInit_Object = null;
		SetDynamicPhysicsLifeTime(0);
		dBodyDestroy(this);
		vector parked = pos;
		parked[1] = g_Game.SurfaceY(pos[0], pos[2]) - PARK_DEPTH;
		SetPosition(parked);

		m_SC_Box = SC_Spawn.PlaceContainer(colour, pos, yaw, SC_Settings.Get().openDoors);
		int loaded = SC_Spawn.Load(m_SC_Box, drop);
		string dropName = "nothing";
		if (drop)
			dropName = drop.name;
		Print("[ShippingCrate] container landed at " + pos + " with " + dropName + " (" + loaded + " things)");

		SC_Config cfg = SC_Settings.Get();
		if (m_SC_Box && cfg.selfDestructMinutes > 0)
		{
			int warnAt = cfg.selfDestructMinutes * 60000;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Warn, warnAt, false);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Detonate, warnAt + cfg.warningSeconds * 1000, false);
		}
	}

	protected void SC_CancelTimers()
	{
		if (!g_Game || !g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM))
			return;
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(SC_Warn);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(SC_Detonate);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(SC_Remove);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(SC_Blast);
	}

	// the teddy bear's eyes light up and it starts telling everyone to get out
	void SC_Warn()
	{
		m_SC_Stage = 1;
		SetSynchDirty();
		Print("[ShippingCrate] container at " + SC_BoxCentre() + ": the bear is warning");
	}

	// C4 on the roof goes off (centre and both ends of the charges), then the container and
	// everything in it are removed: this ends the Expansion mission and frees what it held
	void SC_Detonate()
	{
		m_SC_Stage = 2;
		SetSynchDirty();
		if (!m_SC_Box)
		{
			g_Game.ObjectDelete(this);
			return;
		}
		vector mm[2];
		SC_Spawn.Bounds(m_SC_Box, mm);
		int axis = 2;
		if (mm[1][0] - mm[0][0] > mm[1][2] - mm[0][2])
			axis = 0;
		for (int i = -1; i <= 1; i++)
		{
			vector p = (mm[0] + mm[1]) * 0.5;
			p[1] = mm[1][1] + 0.2;
			p[axis] = p[axis] + i * 1.2;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Blast, 150 * (i + 1), false, m_SC_Box.ModelToWorld(p));
		}
		Print("[ShippingCrate] container at " + SC_BoxCentre() + " blew up");
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Remove, 2000, false);
	}

	void SC_Blast(vector at)
	{
		DamageSystem.ExplosionDamage(this, null, BLAST, at, DamageType.EXPLOSION);
	}

	// everything inside the container goes with it: vehicles and items (and whatever is in
	// them), not people or animals
	void SC_Remove()
	{
		int removed = 0;
		if (m_SC_Box)
		{
			vector mm[2];
			SC_Lights.ClosedBounds(m_SC_Box, mm);
			array<Object> objects = new array<Object>();
			array<CargoBase> proxies = new array<CargoBase>();
			g_Game.GetObjectsAtPosition(SC_BoxCentre(), vector.Distance(mm[0], mm[1]) * 0.5 + 1, objects, proxies);
			foreach (Object obj : objects)
			{
				if (obj == m_SC_Box || obj == this || obj.IsMan() || obj.IsInherited(DayZCreature) || !obj.IsInherited(EntityAI))
					continue;
				EntityAI e = EntityAI.Cast(obj);
				if (e.GetHierarchyParent())
					continue;
				vector rel = m_SC_Box.WorldToModel(obj.GetPosition());
				if (rel[0] < mm[0][0] - 0.2 || rel[0] > mm[1][0] + 0.2 || rel[1] < mm[0][1] - 0.5 || rel[1] > mm[1][1] + 0.5 || rel[2] < mm[0][2] - 0.2 || rel[2] > mm[1][2] + 0.2)
					continue;
				g_Game.ObjectDelete(obj);
				removed++;
			}
		}
		Print("[ShippingCrate] container at " + SC_BoxCentre() + " removed with " + removed + " things in it");
		g_Game.ObjectDelete(this);   // EEDelete takes the container; the mission ends
	}

	vector SC_BoxCentre()
	{
		if (!m_SC_Box)
			return GetPosition();
		vector mm[2];
		SC_Spawn.Bounds(m_SC_Box, mm);
		return m_SC_Box.ModelToWorld((mm[0] + mm[1]) * 0.5);
	}
#endif

	// ---- client ----

	protected void SC_StopFollowing()
	{
		if (m_SC_Following && g_Game && g_Game.GetUpdateQueue(CALL_CATEGORY_GUI))
			g_Game.GetUpdateQueue(CALL_CATEGORY_GUI).Remove(SC_Follow);
		m_SC_Following = false;
	}

	// the falling stand-ins: local container, parachutes, spotlight
	protected void SC_DeleteLocals()
	{
		if (!g_Game)
			return;
		SC_DeleteChutes();
		if (m_SC_Local)
			g_Game.ObjectDelete(m_SC_Local);
		m_SC_Local = null;
		SC_Lights.Destroy(m_SC_Spot);
		m_SC_Spot = null;
	}

	protected void SC_DeleteChutes()
	{
		if (m_SC_ChuteA)
			g_Game.ObjectDelete(m_SC_ChuteA);
		if (m_SC_ChuteB)
			g_Game.ObjectDelete(m_SC_ChuteB);
		m_SC_ChuteA = null;
		m_SC_ChuteB = null;
	}

	protected EntityAI SC_CreateChute()
	{
		EntityAI chute = EntityAI.Cast(g_Game.CreateObjectEx("ExpansionAirdropContainer_Client", GetPosition(), ECE_LOCAL));
		if (chute)
		{
			chute.SetAnimationPhase("camo", 1);       // just the parachute, not the crate under it
			chute.SetAnimationPhase("parachute", 0);
		}
		return chute;
	}

	// the real container the server put where the crate landed
	protected Object SC_FindLanded(vector at)
	{
		vector ground = at;
		ground[1] = g_Game.SurfaceY(at[0], at[2]);
		array<Object> objects = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(ground, 4, objects, proxies);
		foreach (Object obj : objects)
		{
			if (obj.IsInherited(ContainerLockedBase))
				return obj;
		}
		return null;
	}

	void SC_Follow(float dt)
	{
		m_SC_Time += dt;
		SetAnimationPhase("camo", 1);
		SetAnimationPhase("parachute", 1);

		// landed: the server has put the real container up and parked the crate underground
		// until the mission ends. The stand-ins go; the beacons move onto the real container.
		vector at = GetPosition();
		if (m_Expansion_HasDiscardedParachute && at[1] < g_Game.SurfaceY(at[0], at[2]) - 1)
		{
			if (m_SC_Local)
				SC_DeleteLocals();
			if (!m_SC_Landed && m_SC_Time >= m_SC_NextSearch)
			{
				m_SC_NextSearch = m_SC_Time + 0.5;
				m_SC_Landed = SC_FindLanded(at);
			}
			if (m_SC_Landed)
			{
				SC_UpdateBeacons(m_SC_Landed);
				SC_UpdateDecor(m_SC_Landed);
			}
			return;
		}

		if (m_SC_Colour == 0)
			return;   // the colour hasn't arrived yet

		if (!m_SC_Local)
		{
			m_SC_Local = g_Game.CreateObjectEx(SC_Settings.ContainerType(m_SC_Colour), GetPosition(), ECE_LOCAL);
			if (!m_SC_Local)
				return;
		}

		if (m_Expansion_HasDiscardedParachute)
			SC_DeleteChutes();
		else if (!m_SC_ChuteA)
		{
			m_SC_ChuteA = SC_CreateChute();
			m_SC_ChuteB = SC_CreateChute();
		}

		// the container: centred on the crate, bottoms level
		vector crate[2];
		SC_Spawn.Bounds(this, crate);
		vector box[2];
		SC_Spawn.Bounds(m_SC_Local, box);
		vector crateBottom = Vector((crate[0][0] + crate[1][0]) * 0.5, crate[0][1], (crate[0][2] + crate[1][2]) * 0.5);
		vector boxBottom = Vector((box[0][0] + box[1][0]) * 0.5, box[0][1], (box[0][2] + box[1][2]) * 0.5);

		vector mat[4];
		GetTransform(mat);
		mat[3] = ModelToWorld(crateBottom) - VectorToParent(boxBottom);
		m_SC_Local.SetTransform(mat);

		// the spotlight, just under the floor, straight down
		if (!m_SC_Spot)
			m_SC_Spot = SC_Lights.Create(SC_GroundSpot, mat[3]);
		if (m_SC_Spot)
		{
			m_SC_Spot.SetPosition(m_SC_Local.ModelToWorld(boxBottom - "0 0.3 0"));
			m_SC_Spot.SetOrientation(SC_Lights.Aim("0 -1 0"));
		}

		SC_UpdateBeacons(m_SC_Local);
		SC_UpdateDecor(m_SC_Local);

		if (!m_SC_ChuteA)
			return;

		// the parachutes: one over each end of the roof
		int axis = 2;
		if (box[1][0] - box[0][0] > box[1][2] - box[0][2])
			axis = 0;
		float half = (box[1][axis] - box[0][axis]) * 0.5;
		vector chute[2];
		SC_Spawn.Bounds(m_SC_ChuteA, chute);
		vector chuteBottom = Vector((chute[0][0] + chute[1][0]) * 0.5, chute[0][1], (chute[0][2] + chute[1][2]) * 0.5);

		for (int i = 0; i < 2; i++)
		{
			float side = 1;
			EntityAI c = m_SC_ChuteA;
			if (i == 1)
			{
				side = -1;
				c = m_SC_ChuteB;
			}
			vector roof = (box[0] + box[1]) * 0.5;
			roof[1] = box[1][1];
			roof[axis] = roof[axis] + side * half * 0.6;
			mat[3] = m_SC_Local.ModelToWorld(roof) - VectorToParent(chuteBottom);
			c.SetTransform(mat);
		}
	}

	// a local, collision-free copy of type
	protected Object SC_CreateProp(string type, vector pos)
	{
		Object obj = g_Game.CreateObjectEx(type, pos, ECE_LOCAL);
		if (obj)
			dBodyDestroy(obj);
		return obj;
	}

	protected void SC_DeleteDecor()
	{
		if (!g_Game)
			return;
		foreach (Object charge : m_SC_Charges)
		{
			if (charge)
				g_Game.ObjectDelete(charge);
		}
		m_SC_Charges.Clear();
		if (m_SC_Bear)
			g_Game.ObjectDelete(m_SC_Bear);
		if (m_SC_EyeL)
			g_Game.ObjectDelete(m_SC_EyeL);
		if (m_SC_EyeR)
			g_Game.ObjectDelete(m_SC_EyeR);
		m_SC_Bear = null;
		m_SC_EyeL = null;
		m_SC_EyeR = null;
		SC_Lights.Destroy(m_SC_EyeLightL);
		SC_Lights.Destroy(m_SC_EyeLightR);
		m_SC_EyeLightL = null;
		m_SC_EyeLightR = null;
	}

	// On the middle of box's roof: a 4 x 2 grid of IEDs with a teddy bear sitting on top. In its
	// last minute the bear's eyes glow red and it calls out "Evacuate!" every 3 s.
	protected void SC_UpdateDecor(Object box)
	{
		if (m_SC_Stage >= 2)
		{
			SC_DeleteDecor();
			return;
		}
		vector mm[2];
		SC_Lights.ClosedBounds(box, mm);
		int axis = 2;
		int cross = 0;
		if (mm[1][0] - mm[0][0] > mm[1][2] - mm[0][2])
		{
			axis = 0;
			cross = 2;
		}
		vector mat[4];
		box.GetTransform(mat);

		if (m_SC_Charges.Count() == 0)
		{
			for (int c = 0; c < 8; c++)
				m_SC_Charges.Insert(SC_CreateProp(CHARGE, box.GetPosition()));
			m_SC_Bear = SC_CreateProp(BEAR, box.GetPosition());
		}

		// the charges, end to end along the roof, two rows
		vector cm[2];
		cm[0] = "-0.15 0 -0.15";
		cm[1] = "0.15 0.15 0.15";
		if (m_SC_Charges[0])
			SC_Spawn.Bounds(m_SC_Charges[0], cm);
		float stepAlong = (cm[1][axis] - cm[0][axis]) * 1.15;
		float stepAcross = (cm[1][cross] - cm[0][cross]) * 1.15;
		vector charge = Vector((cm[0][0] + cm[1][0]) * 0.5, cm[0][1], (cm[0][2] + cm[1][2]) * 0.5);   // its bottom centre
		for (int i = 0; i < m_SC_Charges.Count(); i++)
		{
			Object obj = m_SC_Charges[i];
			if (!obj)
				continue;
			int col = i % 4;
			int row = i / 4;
			float along = col - 1.5;
			float across = row - 0.5;
			vector p = (mm[0] + mm[1]) * 0.5;
			p[1] = mm[1][1];
			p[axis] = p[axis] + along * stepAlong;
			p[cross] = p[cross] + across * stepAcross;
			mat[3] = box.ModelToWorld(p) - box.VectorToParent(charge);
			obj.SetTransform(mat);
		}

		// the bear sits on top of the middle, facing out the side
		if (!m_SC_Bear)
			return;
		vector bm[2];
		SC_Spawn.Bounds(m_SC_Bear, bm);
		vector top = (mm[0] + mm[1]) * 0.5;
		top[1] = mm[1][1] + (cm[1][1] - cm[0][1]);
		vector bearBottom = Vector((bm[0][0] + bm[1][0]) * 0.5, bm[0][1], (bm[0][2] + bm[1][2]) * 0.5);
		vector bmat[4];
		bmat[0] = mat[0];
		bmat[1] = mat[1];
		bmat[2] = mat[2];
		if (axis == 2)
		{
			// the long axis is z: turn the bear a quarter so it faces across the roof
			bmat[0] = -mat[2];
			bmat[2] = mat[0];
		}
		bmat[3] = box.ModelToWorld(top) - (bmat[0] * bearBottom[0] + bmat[1] * bearBottom[1] + bmat[2] * bearBottom[2]);
		m_SC_Bear.SetTransform(bmat);

		if (!m_SC_LoggedDecor)
		{
			m_SC_LoggedDecor = true;
			Print("[ShippingCrate] roof: charge size " + (cm[1] - cm[0]) + ", bear size " + (bm[1] - bm[0]));
		}

		if (m_SC_Stage == 1)
			SC_Warning(bm);
	}

	// the bear's red eyes (front of its head, about four fifths up) and its voice
	protected void SC_Warning(vector bm[2])
	{
		float h = bm[1][1] - bm[0][1];
		float w = bm[1][0] - bm[0][0];
		float eyeY = bm[0][1] + h * 0.8;
		float eyeZ = bm[1][2] - (bm[1][2] - bm[0][2]) * 0.1;
		float midX = (bm[0][0] + bm[1][0]) * 0.5;
		vector left = m_SC_Bear.ModelToWorld(Vector(midX - w * 0.12, eyeY, eyeZ));
		vector right = m_SC_Bear.ModelToWorld(Vector(midX + w * 0.12, eyeY, eyeZ));

		if (!m_SC_EyeL)
		{
			m_SC_EyeL = SC_Lights.CreateNavMarker("SC_Prop_GlowRed", SC_Lights.NAV_RED, "#(argb,8,8,3)color(1,0,0,1,CO)", left);
			m_SC_EyeR = SC_Lights.CreateNavMarker("SC_Prop_GlowRed", SC_Lights.NAV_RED, "#(argb,8,8,3)color(1,0,0,1,CO)", right);
			m_SC_EyeLightL = SC_Lights.Create(SC_EyeLight, left);
			m_SC_EyeLightR = SC_Lights.Create(SC_EyeLight, right);
			m_SC_NextVoice = m_SC_Time;
		}
		vector mat[4];
		m_SC_Bear.GetTransform(mat);
		if (m_SC_EyeL)
		{
			mat[3] = left;
			m_SC_EyeL.SetTransform(mat);
			m_SC_EyeL.SetScale(0.12);
		}
		if (m_SC_EyeR)
		{
			mat[3] = right;
			m_SC_EyeR.SetTransform(mat);
			m_SC_EyeR.SetScale(0.12);
		}
		if (m_SC_EyeLightL)
			m_SC_EyeLightL.SetPosition(left + m_SC_Bear.VectorToParent("0 0 0.05"));
		if (m_SC_EyeLightR)
			m_SC_EyeLightR.SetPosition(right + m_SC_Bear.VectorToParent("0 0 0.05"));

		if (m_SC_Time >= m_SC_NextVoice)
		{
			m_SC_NextVoice = m_SC_Time + VOICE_EVERY;
			SEffectManager.PlaySound(EVACUATE_SOUND, m_SC_Bear.GetPosition());
		}
	}

	// at each end of box's roof a runway edge light whose lamp is a rotating orange beacon, the
	// two half a turn apart
	protected void SC_UpdateBeacons(Object box)
	{
		vector mm[2];
		SC_Lights.ClosedBounds(box, mm);
		int axis = 2;
		if (mm[1][0] - mm[0][0] > mm[1][2] - mm[0][2])
			axis = 0;
		float half = (mm[1][axis] - mm[0][axis]) * 0.5;

		vector mat[4];
		box.GetTransform(mat);

		for (int i = 0; i < 2; i++)
		{
			float side = 1;
			Object post = m_SC_PostA;
			ScriptedLightBase beam = m_SC_BeamA;
			if (i == 1)
			{
				side = -1;
				post = m_SC_PostB;
				beam = m_SC_BeamB;
			}

			vector roof = (mm[0] + mm[1]) * 0.5;
			roof[1] = mm[1][1];
			roof[axis] = roof[axis] + side * (half - 0.4);
			vector world = box.ModelToWorld(roof);

			if (!post)
			{
				post = SC_Lights.CreatePost(world);
				beam = SC_Lights.Create(SC_BeaconBeam, world);
				if (i == 0)
				{
					m_SC_PostA = post;
					m_SC_BeamA = beam;
				}
				else
				{
					m_SC_PostB = post;
					m_SC_BeamB = beam;
				}
			}

			// the post stands on the roof, upright with the container; its lamp is near the top
			vector lamp = world + box.VectorToParent("0 0.6 0");
			if (post)
			{
				vector pm[2];
				SC_Spawn.Bounds(post, pm);
				mat[3] = world - box.VectorToParent(Vector((pm[0][0] + pm[1][0]) * 0.5, pm[0][1], (pm[0][2] + pm[1][2]) * 0.5));
				post.SetTransform(mat);
				lamp = post.ModelToWorld(Vector((pm[0][0] + pm[1][0]) * 0.5, pm[1][1] - 0.1, (pm[0][2] + pm[1][2]) * 0.5));

				// the lamp: the middle of the dome, the top quarter of the post
				float domeHeight = (pm[1][1] - pm[0][1]) * SC_Lights.LAMP_SHARE;
				lamp = post.ModelToWorld(Vector((pm[0][0] + pm[1][0]) * 0.5, pm[1][1] - domeHeight * 0.5, (pm[0][2] + pm[1][2]) * 0.5));
			}
			vector dir = SC_Lights.BeaconDir(m_SC_Time, i * 180);
			if (beam)
			{
				beam.SetPosition(lamp);
				beam.SetOrientation(SC_Lights.Aim(dir));
			}
		}
	}
}

#ifndef SERVER
#ifdef EXPANSION_MISSIONS_CONTAINER_CLIENT
class ShippingCrate_Airdrop_Client: ExpansionAirdropContainerBase_Client
{
};
#endif
#endif
