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
	ref SC_Gun m_SC_Gun;   // ref: a mix of guns is made for the drop, not in the config
	[NonSerialized()]
	bool m_SC_Blown;   // set by the crate when the self-destruct went off
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

	// Expansion's own "airdrop ended" line says "destroyed by infected" whatever happened; for
	// our drops it's swapped for one that says what did
	override void End()
	{
		ExpansionMissionEventAirdrop airdrop;
		bool ours = m_SC_Active && Class.CastTo(airdrop, this);
		bool notify = false;
		bool shown = false;
		string where = "";
		if (ours)
		{
			shown = airdrop.ShowNotification;
			notify = shown && GetExpansionSettings().GetNotification().ShowAirdropEnded;
			where = airdrop.DropLocation.Name;
			airdrop.ShowNotification = false;
		}

		super.End();

		if (ours)
		{
			airdrop.ShowNotification = shown;
			if (notify)
			{
				string what = "has been cleared away";
				if (m_SC_Blown)
					what = "has self-destructed";
				CreateNotification(new StringLocaliser("The delivery near " + where + " " + what + "."), EXPANSION_NOTIFICATION_ICON_AIRDROP, 7);
			}
			SC_Restore(airdrop);
		}
		m_SC_Active = false;
		m_SC_Blown = false;
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

		// delivery radios in the group pick the vehicle and the guns, each knob on its own
		array<SC_DropRadio> radios = SC_DropRadio.Carried(SC_Spot.GroupPlayers(centre, cfg.clusterRadius));
		string vehicleBy = "random";
		SC_Drop askedDrop = SC_DropRadio.ChooseDrop(radios);
		if (askedDrop)
		{
			drop = askedDrop;
			vehicleBy = "by radio";
		}
		string gunBy = "nobody chose";
		SC_Gun gun = SC_DropRadio.ChooseGun(radios);
		if (gun)
			gunBy = "by radio";
		else
			gun = SC_Settings.MixedGuns();   // "any": the boot full of different guns
		string gunName = "no guns";
		if (gun)
			gunName = gun.name;

		m_SC_SavedLocation = airdrop.DropLocation;
		m_SC_SavedInfectedCount = airdrop.InfectedCount;
		airdrop.DropLocation = new ExpansionAirdropLocation(spot[0], spot[2], cfg.dropRadius, town);
		airdrop.InfectedCount = 0;
		m_SC_Drop = drop;
		m_SC_Gun = gun;
		m_SC_Active = true;
		Print("[ShippingCrate] " + MissionName + ": " + drop.name + " (" + vehicleBy + ") with " + gunName + " (" + gunBy + ") for a group of " + size + " near " + town + " at " + spot);
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
	// The radios once more, as the crate leaves the plane: the load was chosen when the plane
	// set off a couple of minutes ago, and the group may have retuned since. A tuned knob wins;
	// an untuned one keeps what was chosen then.
	protected void SC_ReadRadios()
	{
		SC_Config cfg = SC_Settings.Get();
		vector below = GetPosition();
		below[1] = g_Game.SurfaceY(below[0], below[2]);
		array<SC_DropRadio> radios = SC_DropRadio.Carried(SC_Spot.GroupPlayers(below, cfg.clusterRadius));
		SC_Drop drop = SC_DropRadio.ChooseDrop(radios);
		if (drop && drop != m_SC_Mission.m_SC_Drop)
		{
			Print("[ShippingCrate] retuned since take-off: " + drop.name + " instead of " + m_SC_Mission.m_SC_Drop.name);
			m_SC_Mission.m_SC_Drop = drop;
		}
		SC_Gun gun = SC_DropRadio.ChooseGun(radios);
		if (gun && gun != m_SC_Mission.m_SC_Gun)
		{
			Print("[ShippingCrate] retuned since take-off: " + gun.name + " guns");
			m_SC_Mission.m_SC_Gun = gun;
		}
	}

	override ExpansionAirdropContainerBase Expansion_CreateDrop(string container)
	{
		if (!m_SC_Mission)
			return super.Expansion_CreateDrop(container);

		ExpansionAirdropContainerBase drop = super.Expansion_CreateDrop("ShippingCrate_Airdrop");
		ShippingCrate_Airdrop crate;
		if (Class.CastTo(crate, drop))
		{
			crate.m_SC_Mission = m_SC_Mission;
			SC_ReadRadios();
			crate.SC_Setup(m_SC_Mission.m_SC_Drop, m_SC_Mission.m_SC_Gun);
		}
		return drop;
	}
#endif
}

class ShippingCrate_Airdrop: ExpansionAirdropContainerBase_Server
{
	// synchronised to clients
	int m_SC_Colour;   // 1-based index into SC_Settings.COLOURS, 0 until the server picks one (container only)
	int m_SC_Carrier;  // CARRIER_CONTAINER or CARRIER_PALLET
	int m_SC_Stage;    // the self-destruct: 0 landed (or falling), 1 the bear's warning, 2 blown

	static const int CARRIER_CONTAINER = 0;
	static const int CARRIER_PALLET = 1;

	// The vehicle and its parts, picked when the drop starts. Clients ask for them (RPC) to draw
	// the vehicle on the falling pallet; netsync can't carry strings.
	string m_SC_VehicleType;
	string m_SC_VehicleParts;   // attachment types, ";" separated
	static const int RPC_ASK = 0x5C0A01;
	static const int RPC_INFO = 0x5C0A02;

	// server
	ExpansionMissionEventBase m_SC_Mission;
	ref SC_Drop m_SC_Drop;                             // held, so a load made in script (the test) outlives its maker
	ref SC_Gun m_SC_Gun;
	Building m_SC_Box;                                 // the landed container, if that's the carrier
	EntityAI m_SC_Ride;                                // a pallet's vehicle: the real one, fitted and loaded, carried down on the crate
	int m_SC_RideLow;                                  // synchronised: its network id, so clients can hold it on the
	int m_SC_RideHigh;                                 // falling deck themselves (a sleeping body's moves aren't sent)
	protected ref array<Object> m_SC_Parts = new array<Object>();   // what the carrier is made of: the container, or the deck's pallets
	protected vector m_SC_Frame[4];                    // the landed carrier's frame and bounds (vehicle height included for a pallet)
	protected vector m_SC_Min;
	protected vector m_SC_Max;
	protected float m_SC_BlastY;                       // frame height the C4 goes off at
	protected bool m_SC_Waiting;
	protected bool m_SC_Delivered;
	protected int m_SC_Checks;

	static const float PARK_DEPTH = 3;      // the crate waits this far underground once the carrier is there
	static const float CHUTE_TILT = 20;     // degrees the end parachutes lean away from the middle
	static const float CHUTE_SPREAD = 0.7;  // how far along the carrier they hang: share of half its length

	// client: Expansion's crate stays hidden; in its place a local container (or pallet with the
	// vehicle on it) whose bottom is level with the crate's, parachutes over it, a spotlight
	// underneath, a runway edge light at each end whose lamp is a rotating beacon, and the IEDs
	// and teddy bear. Once it has landed the beacons and the decoration move onto the real thing
	// until the mission ends and the crate goes.
	protected Object m_SC_Local;                                    // falling container
	protected ref array<Object> m_SC_Tiles = new array<Object>();   // falling pallet deck
	protected Object m_SC_LocalVehicle;                             // ... and the vehicle on it: its model only
	protected ref array<EntityAI> m_SC_Chutes = new array<EntityAI>();
	protected ScriptedLightBase m_SC_Spot;
	protected ScriptedLightBase m_SC_BeamA;
	protected ScriptedLightBase m_SC_BeamB;
	protected Object m_SC_PostA;
	protected Object m_SC_PostB;
	protected ref array<Object> m_SC_Charges = new array<Object>();
	protected Object m_SC_Bear;
	protected Object m_SC_EyeL;
	protected Object m_SC_EyeR;
	protected float m_SC_NextVoice;
	protected float m_SC_EyesUntil;
	protected bool m_SC_LoggedDecor;
	protected Object m_SC_Landed;
	protected bool m_SC_Following;
	protected float m_SC_Time;
	protected float m_SC_NextSearch;
	protected float m_SC_NextAsk;

	static const string CHARGE = "SC_Prop_IED";       // config.cpp: the IED model, not an item
	static const string BEAR = "SC_Prop_Bear";        // config.cpp: vanilla's Bear_Beige model, not an item
	static const string EVACUATE_SOUND = "SC_Evacuate_SoundSet";
	static const float VOICE_EVERY = 3;
	static const float EYES_ON = 1.0;                  // seconds the eyes glow with each "Evacuate!" (the clip's length)
	static const string EYE = "SC_Prop_EyeRed";        // config.cpp: a round, gently glowing red ball
	static const float EYE_SIZE = 0.018;               // across, metres
	static const float WIGGLE_TURN = 12;               // degrees either way while it speaks
	static const float WIGGLE_TURN_RATE = 9;           // radians a second
	static const float WIGGLE_ROCK = 7;
	static const float WIGGLE_ROCK_RATE = 14;
	static const float EYES_BACK = 0.060;              // from the front of the bear's bounding box
	static const string BLAST = "Plastic_Explosive_Ammo";
	static const int RUIN_AFTER_MS = 700;   // after the contents' hit: Better Explosions sets a vehicle off within 0.5 s

	void ShippingCrate_Airdrop()
	{
		RegisterNetSyncVariableInt("m_SC_Colour", 0, 8);
		RegisterNetSyncVariableInt("m_SC_Stage", 0, 2);
		RegisterNetSyncVariableInt("m_SC_Carrier", 0, 1);
		RegisterNetSyncVariableInt("m_SC_RideLow");
		RegisterNetSyncVariableInt("m_SC_RideHigh");
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
			// Expansion deletes the crate when the mission ends: the carrier goes too
#ifdef SERVER
			SC_CancelTimers();
#endif
			foreach (Object part : m_SC_Parts)
			{
				if (part)
					g_Game.ObjectDelete(part);
			}
			if (m_SC_Ride)
			{
#ifdef SERVER
				SC_Leftovers.ForgetRide(m_SC_Ride);
#endif
				g_Game.ObjectDelete(m_SC_Ride);   // the crate went before it landed
			}
			m_SC_Ride = null;
			m_SC_Parts.Clear();
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

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		if (rpc_type == RPC_ASK && g_Game.IsDedicatedServer())
		{
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(m_SC_VehicleType);
			rpc.Write(m_SC_VehicleParts);
			rpc.Send(this, RPC_INFO, true, sender);
		}
		else if (rpc_type == RPC_INFO && !g_Game.IsDedicatedServer())
		{
			string type;
			string parts;
			if (ctx.Read(type) && ctx.Read(parts))
			{
				m_SC_VehicleType = type;
				m_SC_VehicleParts = parts;
			}
		}
	}

	bool SC_HasLanded()
	{
		return m_SC_Delivered;
	}

#ifdef SERVER
	// When the plane drops it: the vehicle and its parts picked, and the carrier chosen,
	// "auto" (left out) being the container if the vehicle's body fits it, else the pallet.
	void SC_Setup(SC_Drop drop, SC_Gun gun = null)
	{
		m_SC_Drop = drop;
		m_SC_Gun = gun;
		m_SC_VehicleType = "";
		m_SC_VehicleParts = "";
		if (drop && drop.vehicle && drop.vehicle.type != "")
		{
			m_SC_VehicleType = SC_Settings.Choose(drop.vehicle.type);
			foreach (SC_Item a : drop.vehicle.attachments)
			{
				for (int i = 0; i < SC_Settings.Count(a); i++)
				{
					if (m_SC_VehicleParts != "")
						m_SC_VehicleParts += ";";
					m_SC_VehicleParts += SC_Settings.Choose(a.type);
				}
			}
		}

		string carrier = "";
		if (drop)
			carrier = drop.carrier;
		carrier.ToLower();
		m_SC_Carrier = CARRIER_CONTAINER;
		if (carrier == "pallet")
			m_SC_Carrier = CARRIER_PALLET;
		else if (carrier != "container" && m_SC_VehicleType != "" && !SC_Pallet.FitsContainer(m_SC_VehicleType))
			m_SC_Carrier = CARRIER_PALLET;
		if (m_SC_Carrier == CARRIER_CONTAINER)
			m_SC_Colour = SC_Settings.PickColour();
		SetSynchDirty();
		if (m_SC_Carrier == CARRIER_PALLET && m_SC_VehicleType != "")
			SC_CreateRide();
		Print("[ShippingCrate] dropping " + m_SC_VehicleType + " (body " + SC_Pallet.Size(m_SC_VehicleType) + ") by " + SC_CarrierName());
	}

	string SC_CarrierName()
	{
		if (m_SC_Carrier == CARRIER_PALLET)
			return "pallet";
		return "container";
	}

	void SC_SetColour(int colour)
	{
		m_SC_Colour = colour;
		SetSynchDirty();
	}

	// no loot in the crate, no infected around it, whatever the mission or container settings say;
	// falling fallSpeedScale times as fast as Expansion would have it
	override void Expansion_InitAirdrop(array<ref ExpansionLoot> Loot, TStringArray infected, int ItemCount, int infectedCount, float fallSpeed = 4.5, float windImpact = 0.0)
	{
		super.Expansion_InitAirdrop(new array<ref ExpansionLoot>(), new TStringArray(), 0, 0, fallSpeed * SC_Settings.Get().fallSpeedScale, windImpact);
	}

	// Expansion divides by dt while the crate falls; two updates in one server tick make it 0
	override void Expansion_OnUpdate(float dt)
	{
		if (dt <= 0)
			return;
		super.Expansion_OnUpdate(dt);
		if (m_SC_Ride && !m_SC_Delivered)
			SC_PlaceRide();
	}

	// the crate's own vehicle doesn't count as hitting something (that would pop its parachute)
	override bool Expansion_CanCollideWith(Object obj)
	{
		if (obj && obj == m_SC_Ride)
			return false;
		return super.Expansion_CanCollideWith(obj);
	}

	// A pallet's vehicle rides down for real: made now, fitted, the gun kit in its cargo, gravity
	// off and its body asleep; SC_PlaceRide keeps it on the falling deck, and on landing it's
	// set on the real one and let go. Clients see the actual vehicle all the way down.
	protected void SC_CreateRide()
	{
		EntityAI ride = EntityAI.Cast(g_Game.CreateObjectEx(m_SC_VehicleType, GetPosition() + "0 3 0", ECE_CREATEPHYSICS));
		if (!ride)
		{
			Print("[ShippingCrate] could not create " + m_SC_VehicleType + " to ride down");
			return;
		}
		dBodyEnableGravity(ride, false);
		dBodyActive(ride, ActiveState.INACTIVE);
		if (m_SC_Drop && m_SC_Drop.vehicle)
			SC_Spawn.Fill(ride, m_SC_Drop.vehicle, true);
		if (m_SC_Gun && SC_Spawn.HasBoot(ride))
		{
			SC_Item kit = new SC_Item();
			kit.cargo = m_SC_Gun.cargo;
			SC_Spawn.Fill(ride, kit, true);
		}
		SC_Spawn.KeepLifetime(ride);
		m_SC_Ride = ride;
		SC_Leftovers.RememberRide(m_SC_VehicleType + " on a pallet", SC_RideDropPoint(), ride);
		ride.GetNetworkID(m_SC_RideLow, m_SC_RideHigh);
		SetSynchDirty();
		SC_PlaceRide();
	}

	// the ground under the crate, near enough where it will land (wind drifts it a little)
	protected vector SC_RideDropPoint()
	{
		vector p = GetPosition();
		p[1] = g_Game.SurfaceY(p[0], p[2]);
		return p;
	}

	// the ride on the falling deck: the same frame clients draw the deck in
	protected void SC_PlaceRide()
	{
		vector crate[2];
		SC_Spawn.Bounds(this, crate);
		vector crateBottom = Vector((crate[0][0] + crate[1][0]) * 0.5, crate[0][1], (crate[0][2] + crate[1][2]) * 0.5);
		vector f[4];
		GetTransform(f);
		f[3] = ModelToWorld(crateBottom);
		int nx;
		int nz;
		vector deck[2];
		SC_Pallet.Layout(m_SC_VehicleType, nx, nz, deck);
		SC_Pallet.FromCrate(f, deck, false);
		vector mat[4];
		SC_Pallet.VehicleTransform(f, m_SC_VehicleType, deck, 0.05, mat);
		m_SC_Ride.SetTransform(mat);
		dBodyActive(m_SC_Ride, ActiveState.INACTIVE);
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

	// once the crate has stopped moving (or after 10 s), the carrier takes its place
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
		{
			drop = SC_Settings.Pick();
			SC_Setup(drop, SC_Settings.MixedGuns());
		}

		vector pos = GetPosition();
		float yaw = GetOrientation()[0];
		vector rest[4];
		GetTransform(rest);   // the pallet lines up with the crate's own z axis (see GroundFrame)

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

		int loaded = 0;
		if (m_SC_Carrier == CARRIER_PALLET)
		{
			vector deck[2];
			vector pf[4];
			loaded = SC_Spawn.LoadPallet(drop, m_SC_VehicleType, m_SC_Gun, m_SC_Ride, pos, rest[2], m_SC_Parts, pf, deck);
			SC_Leftovers.ForgetRide(m_SC_Ride);   // the landed pallet's entry covers it from here
			m_SC_Ride = null;   // landed: an ordinary vehicle now
			m_SC_RideLow = 0;
			m_SC_RideHigh = 0;
			SetSynchDirty();
			SC_SetFrame(pf);
			m_SC_Min = deck[0];
			m_SC_Max = deck[1];
			m_SC_Max[1] = deck[1][1] + SC_Pallet.Size(m_SC_VehicleType)[1] + 0.5;
			m_SC_BlastY = deck[1][1] + 0.8;
		}
		else
		{
			int colour = m_SC_Colour;
			if (colour == 0)
				colour = SC_Settings.PickColour();
			m_SC_Box = SC_Spawn.PlaceContainer(colour, pos, yaw, SC_Settings.Get().openDoors);
			if (m_SC_Box)
			{
				m_SC_Parts.Insert(m_SC_Box);
				vector bf[4];
				m_SC_Box.GetTransform(bf);
				SC_SetFrame(bf);
				vector mm[2];
				SC_Spawn.Bounds(m_SC_Box, mm);
				m_SC_Min = mm[0];
				m_SC_Max = mm[1];
				m_SC_BlastY = mm[1][1] + 0.2;
			}
			loaded = SC_Spawn.Load(m_SC_Box, drop, m_SC_VehicleType, m_SC_Gun);
		}
		string dropName = "nothing";
		if (drop)
			dropName = drop.name;
		if (m_SC_Gun)
			dropName += " + " + m_SC_Gun.name;
		Print("[ShippingCrate] " + SC_CarrierName() + " landed at " + pos + " with " + dropName + " (" + loaded + " things)");

		SC_Config cfg = SC_Settings.Get();
		if (m_SC_Parts.Count() > 0 && cfg.selfDestructMinutes > 0)
		{
			int warnAt = cfg.selfDestructMinutes * 60000;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Warn, warnAt, false);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Detonate, warnAt + cfg.warningSeconds * 1000, false);
			// in case the server stops before then (SC_Leftovers.c)
			vector lf[4];
			lf[0] = m_SC_Frame[0];
			lf[1] = m_SC_Frame[1];
			lf[2] = m_SC_Frame[2];
			lf[3] = m_SC_Frame[3];
			SC_Leftovers.Remember(SC_CarrierName() + " with " + dropName, lf, m_SC_Min, m_SC_Max);
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
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(SC_RuinContents);
	}

	// member arrays can't be passed as out parameters: copied in from a local one
	protected void SC_SetFrame(vector f[4])
	{
		m_SC_Frame[0] = f[0];
		m_SC_Frame[1] = f[1];
		m_SC_Frame[2] = f[2];
		m_SC_Frame[3] = f[3];
	}

	vector SC_FrameToWorld(vector p)
	{
		return m_SC_Frame[0] * p[0] + m_SC_Frame[1] * p[1] + m_SC_Frame[2] * p[2] + m_SC_Frame[3];
	}

	vector SC_WorldToFrame(vector w)
	{
		vector d = w - m_SC_Frame[3];
		return Vector(vector.Dot(d, m_SC_Frame[0]), vector.Dot(d, m_SC_Frame[1]), vector.Dot(d, m_SC_Frame[2]));
	}

	vector SC_Centre()
	{
		if (m_SC_Parts.Count() == 0)
			return GetPosition();
		return SC_FrameToWorld((m_SC_Min + m_SC_Max) * 0.5);
	}

	// the teddy bear's eyes light up and it starts telling everyone to get out
	void SC_Warn()
	{
		m_SC_Stage = 1;
		SetSynchDirty();
		Print("[ShippingCrate] " + SC_CarrierName() + " at " + SC_Centre() + ": the bear is warning");
	}

	// C4 goes off (the middle and either side of it, along the carrier), then the carrier and
	// everything on or in it are removed: this ends the Expansion mission and frees what it held
	void SC_Detonate()
	{
		m_SC_Stage = 2;
		SetSynchDirty();
		if (m_SC_Mission)
			m_SC_Mission.m_SC_Blown = true;
		if (m_SC_Parts.Count() == 0)
		{
			g_Game.ObjectDelete(this);
			return;
		}
		int axis = 2;
		if (m_SC_Max[0] - m_SC_Min[0] > m_SC_Max[2] - m_SC_Min[2])
			axis = 0;
		for (int i = -1; i <= 1; i++)
		{
			vector p = (m_SC_Min + m_SC_Max) * 0.5;
			p[1] = m_SC_BlastY;
			p[axis] = p[axis] + i * 1.2;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Blast, 150 * (i + 1), false, SC_FrameToWorld(p));
		}
		int hit = SC_HitContents();
		Print("[ShippingCrate] " + SC_CarrierName() + " at " + SC_Centre() + " blew up, " + hit + " things in or on it hit");
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_RuinContents, RUIN_AFTER_MS, false);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Remove, 2000, false);
	}

	// what is in or on the carrier now
	protected array<EntityAI> SC_Contents()
	{
		vector lf[4];
		lf[0] = m_SC_Frame[0];
		lf[1] = m_SC_Frame[1];
		lf[2] = m_SC_Frame[2];
		lf[3] = m_SC_Frame[3];
		return SC_Leftovers.InBox(lf, m_SC_Min, m_SC_Max, m_SC_Parts);
	}

	// The blast reaches whatever is in or on the carrier however it's shielded (a container's
	// walls stop the area damage): each takes the C4's hit directly, so a fuelled vehicle goes up
	// the way any explosive sets it off (Better Explosions, if the server runs it, wrecks it).
	protected int SC_HitContents()
	{
		array<EntityAI> contents = SC_Contents();
		foreach (EntityAI e : contents)
			e.ProcessDirectDamage(DamageType.EXPLOSION, this, "", BLAST, "0 0 0", 1.0);
		return contents.Count();
	}

	// ... and whatever survived the hit is destroyed anyway (after any vehicle has gone up)
	protected void SC_RuinContents()
	{
		array<EntityAI> contents = SC_Contents();   // held: a returned array isn't kept alive by foreach
		foreach (EntityAI e : contents)
		{
			if (e && !e.IsRuined())
				e.SetHealth("", "", 0);
		}
	}

	void SC_Blast(vector at)
	{
		DamageSystem.ExplosionDamage(this, null, BLAST, at, DamageType.EXPLOSION);
	}

	// everything in or on the carrier goes with it: vehicles and items (and whatever is in them),
	// not people or animals
	void SC_Remove()
	{
		int removed = 0;
		if (m_SC_Parts.Count() > 0)
		{
			vector lf[4];
			lf[0] = m_SC_Frame[0];
			lf[1] = m_SC_Frame[1];
			lf[2] = m_SC_Frame[2];
			lf[3] = m_SC_Frame[3];
			removed = SC_Leftovers.ClearBox(lf, m_SC_Min, m_SC_Max, m_SC_Parts);
			SC_Leftovers.Forget(lf[3]);
		}
		Print("[ShippingCrate] " + SC_CarrierName() + " at " + SC_Centre() + " removed with " + removed + " things on or in it");
		g_Game.ObjectDelete(this);   // EEDelete takes the carrier; the mission ends
	}
#endif

	// ---- client ----

	protected void SC_StopFollowing()
	{
		if (m_SC_Following && g_Game && g_Game.GetUpdateQueue(CALL_CATEGORY_GUI))
			g_Game.GetUpdateQueue(CALL_CATEGORY_GUI).Remove(SC_Follow);
		m_SC_Following = false;
	}

	// the falling stand-ins: local container or pallet and vehicle, parachutes, spotlight
	protected void SC_DeleteLocals()
	{
		if (!g_Game)
			return;
		SC_DeleteChutes();
		if (m_SC_Local)
			g_Game.ObjectDelete(m_SC_Local);
		m_SC_Local = null;
		foreach (Object tile : m_SC_Tiles)
		{
			if (tile)
				g_Game.ObjectDelete(tile);
		}
		m_SC_Tiles.Clear();
		if (m_SC_LocalVehicle)
			g_Game.ObjectDelete(m_SC_LocalVehicle);
		m_SC_LocalVehicle = null;
		SC_Lights.Destroy(m_SC_Spot);
		m_SC_Spot = null;
	}

	protected void SC_DeleteChutes()
	{
		foreach (EntityAI chute : m_SC_Chutes)
		{
			if (chute)
				g_Game.ObjectDelete(chute);
		}
		m_SC_Chutes.Clear();
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

	// a local, collision-free copy of type
	protected Object SC_CreateProp(string type, vector pos)
	{
		Object obj = g_Game.CreateObjectEx(type, pos, ECE_LOCAL);
		if (obj)
			dBodyDestroy(obj);
		return obj;
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

	static vector SC_Dir(vector f[4], vector d)
	{
		return f[0] * d[0] + f[1] * d[1] + f[2] * d[2];
	}

	void SC_Follow(float dt)
	{
		m_SC_Time += dt;
		SetAnimationPhase("camo", 1);
		SetAnimationPhase("parachute", 1);

		// a pallet needs its vehicle's type, which comes from the server on request
		if (m_SC_Carrier == CARRIER_PALLET && m_SC_VehicleType == "")
		{
			if (m_SC_Time >= m_SC_NextAsk)
			{
				m_SC_NextAsk = m_SC_Time + 3;
				g_Game.RPCSingleParam(this, RPC_ASK, new Param1<int>(0), true, null);
			}
			return;
		}

		vector f[4];
		vector mm[2];
		vector deck[2];
		int nx;
		int nz;
		if (m_SC_Carrier == CARRIER_PALLET)
			SC_Pallet.Layout(m_SC_VehicleType, nx, nz, deck);

		// landed: the server has put the real carrier down and parked the crate underground
		// until the mission ends. The stand-ins go; beacons and decoration move onto the real one.
		vector at = GetPosition();
		if (m_Expansion_HasDiscardedParachute && at[1] < g_Game.SurfaceY(at[0], at[2]) - 1)
		{
			if (m_SC_Local || m_SC_Tiles.Count() > 0)
				SC_DeleteLocals();
			if (m_SC_Carrier == CARRIER_PALLET)
			{
				vector crateMat[4];
				GetTransform(crateMat);
				SC_Pallet.GroundFrame(at, crateMat[2], f);
				SC_Pallet.FromCrate(f, deck, true);
				SC_Dress(f, deck, true);
				return;
			}
			if (!m_SC_Landed && m_SC_Time >= m_SC_NextSearch)
			{
				m_SC_NextSearch = m_SC_Time + 0.5;
				m_SC_Landed = SC_FindLanded(at);
			}
			if (m_SC_Landed)
			{
				m_SC_Landed.GetTransform(f);
				SC_Lights.ClosedBounds(m_SC_Landed, mm);
				SC_Dress(f, mm, false);
			}
			return;
		}

		// falling: the carrier hangs with its bottom level with the crate's, centred on it
		vector crate[2];
		SC_Spawn.Bounds(this, crate);
		vector crateBottom = Vector((crate[0][0] + crate[1][0]) * 0.5, crate[0][1], (crate[0][2] + crate[1][2]) * 0.5);
		GetTransform(f);
		float chuteTop;
		int chuteCount = 2;

		if (m_SC_Carrier == CARRIER_PALLET)
		{
			f[3] = ModelToWorld(crateBottom);
			SC_Pallet.FromCrate(f, deck, false);   // the crate is under the deck's free end
			SC_FollowPallet(f, nx, nz, deck);
			mm[0] = deck[0];
			mm[1] = deck[1];
			chuteTop = deck[1][1] + SC_Pallet.Size(m_SC_VehicleType)[1] + 0.2;
			chuteCount = Math.Clamp(Math.Round((deck[1][2] - deck[0][2]) / 4.5), 2, 6);
		}
		else
		{
			if (m_SC_Colour == 0)
				return;   // the colour hasn't arrived yet
			if (!m_SC_Local)
			{
				m_SC_Local = g_Game.CreateObjectEx(SC_Settings.ContainerType(m_SC_Colour), GetPosition(), ECE_LOCAL);
				if (!m_SC_Local)
					return;
			}
			SC_Lights.ClosedBounds(m_SC_Local, mm);
			vector boxBottom = Vector((mm[0][0] + mm[1][0]) * 0.5, mm[0][1], (mm[0][2] + mm[1][2]) * 0.5);
			f[3] = ModelToWorld(crateBottom) - VectorToParent(boxBottom);
			m_SC_Local.SetTransform(f);
			chuteTop = mm[1][1];
		}

		// the spotlight, just under the floor, straight down
		vector under = (mm[0] + mm[1]) * 0.5;
		under[1] = mm[0][1] - 0.3;
		if (!m_SC_Spot)
			m_SC_Spot = SC_Lights.Create(SC_GroundSpot, f[3]);
		if (m_SC_Spot)
		{
			m_SC_Spot.SetPosition(SC_Pallet.ToWorld(f, under));
			m_SC_Spot.SetOrientation(SC_Lights.Aim("0 -1 0"));
		}

		SC_Dress(f, mm, m_SC_Carrier == CARRIER_PALLET);

		if (m_Expansion_HasDiscardedParachute)
			SC_DeleteChutes();
		else
			SC_UpdateChutes(f, mm, chuteTop, chuteCount);
	}

	// the falling pallet: its tiles and the vehicle on it, in frame f
	protected void SC_FollowPallet(vector f[4], int nx, int nz, vector deck[2])
	{
		if (m_SC_Tiles.Count() == 0)
		{
			for (int k = 0; k < nx * nz; k++)
				m_SC_Tiles.Insert(SC_CreateProp(SC_Pallet.TILE, GetPosition()));
			// no stand-in for the vehicle: the real one rides down (the server carries it)
		}
		vector mat[4];
		for (int i = 0; i < nx; i++)
		{
			for (int j = 0; j < nz; j++)
			{
				Object tile = m_SC_Tiles[i * nz + j];
				if (!tile)
					continue;
				SC_Pallet.TileTransform(f, i, j, deck, mat);
				tile.SetTransform(mat);
			}
		}
		if (m_SC_LocalVehicle)
		{
			SC_Pallet.VehicleTransform(f, m_SC_VehicleType, deck, 0, mat);
			m_SC_LocalVehicle.SetTransform(mat);
		}

		// the real vehicle riding down: the server carries it with its body asleep, and those
		// moves never reach us, so hold it on the deck here too (the same place the server has it)
		if (m_SC_RideLow != 0 || m_SC_RideHigh != 0)
		{
			Object ride = g_Game.GetObjectByNetworkId(m_SC_RideLow, m_SC_RideHigh);
			if (ride)
			{
				SC_Pallet.VehicleTransform(f, m_SC_VehicleType, deck, 0.05, mat);
				ride.SetTransform(mat);
			}
		}
	}

	// count parachutes in a row along the carrier, chuteTop above its frame's origin; the end
	// ones lean CHUTE_TILT away from the middle, the ones between proportionally less
	protected void SC_UpdateChutes(vector f[4], vector mm[2], float chuteTop, int count)
	{
		if (m_SC_Chutes.Count() != count)
		{
			SC_DeleteChutes();
			for (int k = 0; k < count; k++)
				m_SC_Chutes.Insert(SC_CreateChute());
		}
		if (!m_SC_Chutes[0])
			return;
		int axis = 2;
		if (mm[1][0] - mm[0][0] > mm[1][2] - mm[0][2])
			axis = 0;
		float reach = (mm[1][axis] - mm[0][axis]) * 0.5 * CHUTE_SPREAD;
		vector chute[2];
		SC_Spawn.Bounds(m_SC_Chutes[0], chute);
		vector chuteBottom = Vector((chute[0][0] + chute[1][0]) * 0.5, chute[0][1], (chute[0][2] + chute[1][2]) * 0.5);

		for (int i = 0; i < count; i++)
		{
			EntityAI c = m_SC_Chutes[i];
			if (!c)
				continue;
			float share = (i / (count - 1.0)) * 2 - 1;   // -1 .. 1 along the carrier
			float tilt = CHUTE_TILT * share * Math.DEG2RAD;
			float tc = Math.Cos(tilt);
			float ts = Math.Sin(tilt);
			vector cmat[4];
			cmat[0] = f[0];
			cmat[1] = f[1];
			cmat[2] = f[2];
			vector up = f[1];
			vector along = f[axis];
			cmat[1] = up * tc + along * ts;
			cmat[axis] = along * tc - up * ts;
			vector top = (mm[0] + mm[1]) * 0.5;
			top[1] = chuteTop;
			top[axis] = top[axis] + share * reach;
			vector attach = cmat[0] * chuteBottom[0] + cmat[1] * chuteBottom[1] + cmat[2] * chuteBottom[2];
			cmat[3] = SC_Pallet.ToWorld(f, top) - attach;
			c.SetTransform(cmat);
		}
	}

	// the beacons and the decoration, on a container's roof or a pallet's deck
	protected void SC_Dress(vector f[4], vector mm[2], bool pallet)
	{
		float crossOffset = 0;
		if (pallet)
			crossOffset = (mm[1][0] - mm[0][0]) * 0.5 - 0.3;   // the deck's corners, clear of the vehicle
		SC_UpdateBeacons(f, mm, crossOffset);
		SC_UpdateDecor(f, mm, pallet);
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
		m_SC_Bear = null;
		SC_DeleteEyes();
	}

	// A stack of IEDs with a teddy bear sitting on top: in the middle of a container's roof, or at
	// the free end of a pallet (tall enough that a slope doesn't bury it). In its last minute the
	// bear's eyes glow red and it calls out "Evacuate!" every 3 s.
	protected void SC_UpdateDecor(vector f[4], vector mm[2], bool pallet)
	{
		if (m_SC_Stage >= 2)
		{
			SC_DeleteDecor();
			return;
		}
		int axis = 2;
		int cross = 0;
		if (mm[1][0] - mm[0][0] > mm[1][2] - mm[0][2])
		{
			axis = 0;
			cross = 2;
		}
		vector base = (mm[0] + mm[1]) * 0.5;
		base[1] = mm[1][1];
		if (pallet)
			base[axis] = mm[1][axis] - 0.45;   // the free end's edge

		if (m_SC_Charges.Count() == 0)
		{
			for (int c = 0; c < 8; c++)
				m_SC_Charges.Insert(SC_CreateProp(CHARGE, f[3]));
			m_SC_Bear = SC_CreateProp(BEAR, f[3]);
		}

		vector mat[4];
		mat[0] = f[0];
		mat[1] = f[1];
		mat[2] = f[2];

		vector cm[2];
		cm[0] = "-0.15 0 -0.15";
		cm[1] = "0.15 0.15 0.15";
		if (m_SC_Charges[0])
			SC_Spawn.Bounds(m_SC_Charges[0], cm);
		// the charges stacked straight up, every other one turned a quarter, like a pile of bricks
		float layer = cm[1][1] - cm[0][1];
		vector charge = Vector((cm[0][0] + cm[1][0]) * 0.5, cm[0][1], (cm[0][2] + cm[1][2]) * 0.5);   // its bottom centre
		for (int i = 0; i < m_SC_Charges.Count(); i++)
		{
			Object obj = m_SC_Charges[i];
			if (!obj)
				continue;
			vector cmat[4];
			cmat[0] = f[0];
			cmat[1] = f[1];
			cmat[2] = f[2];
			if (i % 2 == 1)
			{
				cmat[0] = -f[2];
				cmat[2] = f[0];
			}
			vector p = base;
			p[1] = base[1] + i * layer;
			cmat[3] = SC_Pallet.ToWorld(f, p) - (cmat[0] * charge[0] + cmat[1] * charge[1] + cmat[2] * charge[2]);
			obj.SetTransform(cmat);
		}

		// the bear sits on top of the stack, facing across
		if (!m_SC_Bear)
			return;
		vector bm[2];
		SC_Spawn.Bounds(m_SC_Bear, bm);
		vector top = base;
		top[1] = base[1] + m_SC_Charges.Count() * layer;
		vector bearBottom = Vector((bm[0][0] + bm[1][0]) * 0.5, bm[0][1], (bm[0][2] + bm[1][2]) * 0.5);
		vector bmat[4];
		bmat[0] = f[0];
		bmat[1] = f[1];
		bmat[2] = f[2];
		if (axis == 2 && !pallet)
		{
			// the long axis is z: turn the bear a quarter so it faces across the roof
			bmat[0] = -f[2];
			bmat[2] = f[0];
		}
		if (m_SC_Stage == 1 && m_SC_Time < m_SC_EyesUntil)
			SC_Wiggle(bmat);   // while it speaks, turning and rocking on its bottom
		bmat[3] = SC_Pallet.ToWorld(f, top) - (bmat[0] * bearBottom[0] + bmat[1] * bearBottom[1] + bmat[2] * bearBottom[2]);
		m_SC_Bear.SetTransform(bmat);

		if (!m_SC_LoggedDecor)
		{
			m_SC_LoggedDecor = true;
			Print("[ShippingCrate] decoration: charge size " + (cm[1] - cm[0]) + ", bear size " + (bm[1] - bm[0]) + ", on a " + SC_ClientCarrierName());
		}

		if (m_SC_Stage == 1)
			SC_Warning(bm);
	}

	// the bear's axes, turned a little from side to side and rocked (two rates, so it doesn't
	// look mechanical); its position is worked out from them afterwards
	protected void SC_Wiggle(inout vector m[4])
	{
		float turn = Math.Sin(m_SC_Time * WIGGLE_TURN_RATE) * WIGGLE_TURN * Math.DEG2RAD;
		float rock = Math.Sin(m_SC_Time * WIGGLE_ROCK_RATE) * WIGGLE_ROCK * Math.DEG2RAD;
		vector x = m[0] * Math.Cos(turn) + m[2] * Math.Sin(turn);
		vector z = m[2] * Math.Cos(turn) - m[0] * Math.Sin(turn);
		vector y = m[1];
		m[0] = x * Math.Cos(rock) + y * Math.Sin(rock);
		m[1] = y * Math.Cos(rock) - x * Math.Sin(rock);
		m[2] = z;
	}

	string SC_ClientCarrierName()
	{
		if (m_SC_Carrier == CARRIER_PALLET)
			return "pallet (" + m_SC_VehicleType + ")";
		return "container";
	}

	// the bear's voice every VOICE_EVERY seconds, its eyes (front of its head, about four fifths
	// up) glowing red only while it speaks
	protected void SC_Warning(vector bm[2])
	{
		if (m_SC_Time >= m_SC_NextVoice)
		{
			m_SC_NextVoice = m_SC_Time + VOICE_EVERY;
			m_SC_EyesUntil = m_SC_Time + EYES_ON;
			SEffectManager.PlaySound(EVACUATE_SOUND, m_SC_Bear.GetPosition());
		}

		if (m_SC_Time >= m_SC_EyesUntil)
		{
			SC_DeleteEyes();
			return;
		}

		float h = bm[1][1] - bm[0][1];
		float w = bm[1][0] - bm[0][0];
		float eyeY = bm[0][1] + h * 0.8;
		float eyeZ = bm[1][2] - (bm[1][2] - bm[0][2]) * 0.1 - EYES_BACK;   // the head is behind the paws
		float midX = (bm[0][0] + bm[1][0]) * 0.5;
		vector left = m_SC_Bear.ModelToWorld(Vector(midX - w * 0.12, eyeY, eyeZ));
		vector right = m_SC_Bear.ModelToWorld(Vector(midX + w * 0.12, eyeY, eyeZ));
		if (!m_SC_EyeL)
		{
			m_SC_EyeL = SC_CreateProp(EYE, left);
			m_SC_EyeR = SC_CreateProp(EYE, right);
		}
		// a plum scaled down to EYE_SIZE across, centred on each eye
		float eyeScale = 0.4;
		vector em[2];
		if (m_SC_EyeL)
		{
			SC_Spawn.Bounds(m_SC_EyeL, em);
			float across = Math.Max(em[1][0] - em[0][0], 0.01);
			eyeScale = EYE_SIZE / across;
		}
		vector eyeCentre = (em[0] + em[1]) * 0.5 * eyeScale;
		vector mat[4];
		m_SC_Bear.GetTransform(mat);
		if (m_SC_EyeL)
		{
			mat[3] = left - (mat[0] * eyeCentre[0] + mat[1] * eyeCentre[1] + mat[2] * eyeCentre[2]);
			m_SC_EyeL.SetTransform(mat);
			m_SC_EyeL.SetScale(eyeScale);
		}
		if (m_SC_EyeR)
		{
			mat[3] = right - (mat[0] * eyeCentre[0] + mat[1] * eyeCentre[1] + mat[2] * eyeCentre[2]);
			m_SC_EyeR.SetTransform(mat);
			m_SC_EyeR.SetScale(eyeScale);
		}
	}

	protected void SC_DeleteEyes()
	{
		if (m_SC_EyeL)
			g_Game.ObjectDelete(m_SC_EyeL);
		if (m_SC_EyeR)
			g_Game.ObjectDelete(m_SC_EyeR);
		m_SC_EyeL = null;
		m_SC_EyeR = null;
	}

	// at each end of the carrier a runway edge light whose lamp is a rotating orange beacon, the
	// two half a turn apart; crossOffset moves them sideways, in opposite directions (a pallet's
	// diagonal corners, clear of the vehicle)
	protected void SC_UpdateBeacons(vector f[4], vector mm[2], float crossOffset)
	{
		int axis = 2;
		int cross = 0;
		if (mm[1][0] - mm[0][0] > mm[1][2] - mm[0][2])
		{
			axis = 0;
			cross = 2;
		}
		float half = (mm[1][axis] - mm[0][axis]) * 0.5;

		vector mat[4];
		mat[0] = f[0];
		mat[1] = f[1];
		mat[2] = f[2];

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
			roof[cross] = roof[cross] + side * crossOffset;
			vector world = SC_Pallet.ToWorld(f, roof);

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

			// the post stands upright with the carrier; its lamp is the middle of the dome, the
			// top quarter of the post
			vector lamp = world + SC_Dir(f, "0 0.6 0");
			if (post)
			{
				vector pm[2];
				SC_Spawn.Bounds(post, pm);
				mat[3] = world - SC_Dir(f, Vector((pm[0][0] + pm[1][0]) * 0.5, pm[0][1], (pm[0][2] + pm[1][2]) * 0.5));
				post.SetTransform(mat);
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
