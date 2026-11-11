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

	void ShippingCrate_Airdrop()
	{
		RegisterNetSyncVariableInt("m_SC_Colour", 0, 8);
	}

#ifdef SERVER
	ExpansionMissionEventBase m_SC_Mission;
	SC_Drop m_SC_Drop;
	Building m_SC_Box;
	protected bool m_SC_Waiting;
	protected bool m_SC_Delivered;
	protected int m_SC_Checks;

	static const float PARK_DEPTH = 3;   // the crate waits this far underground once the container is there

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
	}

	// Expansion deletes the crate when the mission ends: the container goes too
	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (m_SC_Box)
			g_Game.ObjectDelete(m_SC_Box);
		m_SC_Box = null;
	}
#else
	// Client: Expansion's crate stays hidden; in its place a local shipping container whose
	// floor is level with the crate's bottom, and a parachute over each end of it.
	protected Object m_SC_Box;
	protected EntityAI m_SC_ChuteA;
	protected EntityAI m_SC_ChuteB;
	protected bool m_SC_Following;

	override void EEInit()
	{
		super.EEInit();
		m_SC_Following = true;
		g_Game.GetUpdateQueue(CALL_CATEGORY_GUI).Insert(SC_Follow);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		SC_StopFollowing();
		SC_DeleteLocals();
	}

	// Only unhooks: by the time the destructor runs (e.g. leaving the server) the local objects
	// may already be gone, and deleting them from here crashes the game.
	void ~ShippingCrate_Airdrop()
	{
		SC_StopFollowing();
	}

	protected void SC_StopFollowing()
	{
		if (m_SC_Following && g_Game && g_Game.GetUpdateQueue(CALL_CATEGORY_GUI))
			g_Game.GetUpdateQueue(CALL_CATEGORY_GUI).Remove(SC_Follow);
		m_SC_Following = false;
	}

	protected void SC_DeleteLocals()
	{
		if (!g_Game)
			return;
		SC_DeleteChutes();
		if (m_SC_Box)
			g_Game.ObjectDelete(m_SC_Box);
		m_SC_Box = null;
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

	void SC_Follow(float dt)
	{
		SetAnimationPhase("camo", 1);
		SetAnimationPhase("parachute", 1);

		// landed: the server has put the real container up and parked the crate underground
		// until the mission ends, so the stand-ins go now
		vector at = GetPosition();
		if (m_Expansion_HasDiscardedParachute && at[1] < g_Game.SurfaceY(at[0], at[2]) - 1)
		{
			SC_StopFollowing();
			SC_DeleteLocals();
			return;
		}

		if (m_SC_Colour == 0)
			return;   // the colour hasn't arrived yet

		if (!m_SC_Box)
		{
			m_SC_Box = g_Game.CreateObjectEx(SC_Settings.ContainerType(m_SC_Colour), GetPosition(), ECE_LOCAL);
			if (!m_SC_Box)
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
		SC_Spawn.Bounds(m_SC_Box, box);
		vector crateBottom = Vector((crate[0][0] + crate[1][0]) * 0.5, crate[0][1], (crate[0][2] + crate[1][2]) * 0.5);
		vector boxBottom = Vector((box[0][0] + box[1][0]) * 0.5, box[0][1], (box[0][2] + box[1][2]) * 0.5);

		vector mat[4];
		GetTransform(mat);
		mat[3] = ModelToWorld(crateBottom) - VectorToParent(boxBottom);
		m_SC_Box.SetTransform(mat);

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
			mat[3] = m_SC_Box.ModelToWorld(roof) - VectorToParent(chuteBottom);
			c.SetTransform(mat);
		}
	}
#endif
}

#ifndef SERVER
#ifdef EXPANSION_MISSIONS_CONTAINER_CLIENT
class ShippingCrate_Airdrop_Client: ExpansionAirdropContainerBase_Client
{
};
#endif
#endif
