// The drop's lights. Client side only (lights are script entities, no objects or collision):
// navigation lights under the plane's wingtips and at its tail, a spotlight under the falling
// container shining on where it will land, at each end of the container a runway edge light
// whose lamp is a rotating orange beacon, falling and landed, (the teddy bear's eyes are a glowing prop, not a light).

// wingtip navigation light: red on the left, green on the right
class SC_NavLight: PointLightBase
{
	void SC_NavLight()
	{
		SetVisibleDuringDaylight(true);
		SetRadiusTo(25);     // big: it lights up the underside of the wing in its colour
		SetBrightnessTo(40);
		SetCastShadow(false);
		EnableSpecular(false);
		SetFlareVisible(true);
	}
}

// under the falling container, pointing at the ground
class SC_GroundSpot: SpotLightBase
{
	void SC_GroundSpot()
	{
		SetVisibleDuringDaylight(true);
		SetRadiusTo(2000);     // well past the 450 m drop height
		SetSpotLightAngle(60);
		SetBrightnessTo(120);
		SetCastShadow(false);
		SetFlareVisible(true);
		SetAmbientColor(1.0, 0.97, 0.9);
		SetDiffuseColor(1.0, 0.97, 0.9);
	}
}

// the beam of a rotating beacon
class SC_BeaconBeam: SpotLightBase
{
	void SC_BeaconBeam()
	{
		SetVisibleDuringDaylight(true);
		SetRadiusTo(70);
		SetSpotLightAngle(25);
		SetBrightnessTo(8);
		SetCastShadow(false);
		SetFlareVisible(true);
		SetAmbientColor(1.0, 0.45, 0.05);
		SetDiffuseColor(1.0, 0.45, 0.05);
	}
}

class SC_Lights
{
	static const string BEACON_POST = "SC_RunwayLight";   // the airfield's runway edge light, its dome glowing orange (config.cpp)

	static const float BEACON_TURN = 240;    // degrees a second
	static const float BEACON_TILT = -0.08;  // the beam dips slightly below level

	static ScriptedLightBase Create(typename type, vector pos)
	{
		if (g_Game.IsDedicatedServer())
			return null;
		return ScriptedLightBase.CreateLight(type, pos);
	}

	static void Destroy(ScriptedLightBase light)
	{
		if (light)
			light.Destroy();
	}

	// a local copy of the runway edge light, without collision
	static Object CreatePost(vector pos)
	{
		if (g_Game.IsDedicatedServer())
			return null;
		Object post = g_Game.CreateObjectEx(BEACON_POST, pos, ECE_LOCAL);
		if (post)
			dBodyDestroy(post);
		return post;
	}

	// Navigation markers: lights 450 m up don't get drawn (the engine skips dynamic lights whose
	// glow doesn't come near the camera), so each wingtip also carries something that is drawn
	// at any distance: a chemlight's shape (SC_Prop_Glow*, config.cpp), scaled up, in a solid
	// colour with a strongly glowing material.
	static const float NAV_MARKER_SCALE = 25;
	static const string NAV_RED = "ShippingCrate\\data\\sc_nav_red.rvmat";
	static const string NAV_GREEN = "ShippingCrate\\data\\sc_nav_green.rvmat";
	static const string NAV_WHITE = "ShippingCrate\\data\\sc_nav_white.rvmat";

	static const float LAMP_SHARE = 0.25;    // the runway light's dome: the top quarter of the post

	static Object CreateNavMarker(string type, string material, string texture, vector pos)
	{
		return CreateDressed(type, material, texture, pos);
	}

	// a local, collision-free copy of type, its colour and material set again for good measure
	static Object CreateDressed(string type, string material, string texture, vector pos)
	{
		if (g_Game.IsDedicatedServer())
			return null;
		EntityAI e = EntityAI.Cast(g_Game.CreateObjectEx(type, pos, ECE_LOCAL));
		if (!e)
			return null;
		dBodyDestroy(e);
		Dress(e, material, texture);
		return e;
	}

	static void Dress(Object obj, string material, string texture)
	{
		EntityAI e;
		if (!Class.CastTo(e, obj))
			return;
		e.SetObjectTexture(0, texture);
		e.SetObjectMaterial(0, material);
	}


	// Bounds of a container type with its doors shut, measured once on a throwaway local copy:
	// a real container's collision box grows when a door swings open.
	protected static ref map<string, ref array<vector>> s_ClosedBounds = new map<string, ref array<vector>>();

	static void ClosedBounds(Object box, out vector mm[2])
	{
		array<vector> known;
		if (!s_ClosedBounds.Find(box.GetType(), known))
		{
			known = new array<vector>();
			Object probe = g_Game.CreateObjectEx(box.GetType(), box.GetPosition() - "0 50 0", ECE_LOCAL);
			vector pm[2];
			if (probe)
			{
				SC_Spawn.Bounds(probe, pm);
				g_Game.ObjectDelete(probe);
			}
			else
				SC_Spawn.Bounds(box, pm);
			known.Insert(pm[0]);
			known.Insert(pm[1]);
			s_ClosedBounds.Insert(box.GetType(), known);
		}
		mm[0] = known[0];
		mm[1] = known[1];
	}

	// direction -> orientation for SetOrientation (lights shine along their forward axis)
	static vector Aim(vector dir)
	{
		return dir.VectorToAngles();
	}

	// beam direction of a beacon at time t; phase in degrees
	static vector BeaconDir(float t, float phase)
	{
		float a = (t * BEACON_TURN + phase) * Math.DEG2RAD;
		return Vector(Math.Sin(a), BEACON_TILT, Math.Cos(a)).Normalized();
	}
}
