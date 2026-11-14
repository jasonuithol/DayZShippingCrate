// Where the delivery goes: where the player ordered it, moved to the nearest flat, dry, clear
// ground big enough for it.
class SC_Spot
{
	static const float CLEAR_RADIUS = 9;     // nothing solid this close (the container is ~6 m long)
	static const float RING_STEP = 15;
	static const float EDGE_MARGIN = 100;    // stay this far inside the map edge

	static array<vector> PlayerPositions()
	{
		array<vector> positions = new array<vector>();
		array<Man> men = new array<Man>();
		g_Game.GetPlayers(men);
		foreach (Man man : men)
		{
			PlayerBase player;
			if (Class.CastTo(player, man) && player.IsAlive() && player.GetIdentity())
				positions.Insert(player.GetPosition());
		}
		return positions;
	}

	static float Dist2D2(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}

	// Rings outwards from centre until a spot passes Clear(); centre itself if none does.
	static vector FlatGround(vector centre, float searchRadius, float maxSlope)
	{
		for (float r = 0; r <= searchRadius; r += RING_STEP)
		{
			int steps = 1;
			if (r > 0)
				steps = Math.Max(8, Math.Round(Math.PI2 * r / RING_STEP));
			float start = Math.RandomFloat(0, Math.PI2);
			for (int s = 0; s < steps; s++)
			{
				float a = start + Math.PI2 * s / steps;
				vector p = Vector(centre[0] + Math.Cos(a) * r, 0, centre[2] + Math.Sin(a) * r);
				p[1] = g_Game.SurfaceY(p[0], p[2]);
				if (Clear(p, maxSlope))
					return p;
			}
		}
		Print("[ShippingCrate] no flat, clear ground within " + searchRadius + " m of " + centre + ", dropping there anyway");
		return centre;
	}

	static bool Clear(vector p, float maxSlope)
	{
		float size = g_Game.GetWorld().GetWorldSize();
		if (p[0] < EDGE_MARGIN || p[2] < EDGE_MARGIN || p[0] > size - EDGE_MARGIN || p[2] > size - EDGE_MARGIN)
			return false;

		// dry land, and the ground under the whole footprint within maxSlope of level
		float h = CLEAR_RADIUS * 0.7;
		float lowest = p[1];
		float highest = p[1];
		for (int i = 0; i < 8; i++)
		{
			float a = Math.PI2 * i / 8;
			float x = p[0] + Math.Cos(a) * h;
			float z = p[2] + Math.Sin(a) * h;
			if (g_Game.SurfaceIsSea(x, z) || g_Game.SurfaceIsPond(x, z))
				return false;
			float y = g_Game.SurfaceY(x, z);
			lowest = Math.Min(lowest, y);
			highest = Math.Max(highest, y);
		}
		if (g_Game.SurfaceIsSea(p[0], p[2]) || g_Game.SurfaceIsPond(p[0], p[2]))
			return false;
		float slope = Math.Atan2(highest - lowest, 2 * h) * Math.RAD2DEG;
		if (slope > maxSlope)
			return false;

		// nothing solid in the way: trees, buildings, walls, rocks, vehicles, players (the
		// falling crate crushes whoever it lands on). Bushes, loose items and animals are fine.
		array<Object> objects = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(p, CLEAR_RADIUS, objects, proxies);
		foreach (Object obj : objects)
		{
			if (obj.IsBush() || obj.IsInherited(ItemBase) || obj.IsInherited(DayZCreature))
				continue;
			return false;
		}
		return true;
	}

	static const float WATER_CLEAR = 8;      // open water this far around a boat's spot
	static const float WATER_DEPTH = 1.5;    // and at least this deep under it

	// The nearest open water within searchRadius of centre: sea or pond under the spot and all
	// round it, deep enough, and nothing solid there (rocks, piers, wrecks, other boats). Its
	// height is the water's surface. False if there's none.
	static bool WaterSpot(vector centre, float searchRadius, out vector spot)
	{
		for (float r = 0; r <= searchRadius; r += 5)
		{
			int steps = 1;
			if (r > 0)
				steps = Math.Max(8, Math.Round(Math.PI2 * r / 5));
			float start = Math.RandomFloat(0, Math.PI2);
			for (int s = 0; s < steps; s++)
			{
				float a = start + Math.PI2 * s / steps;
				float x = centre[0] + Math.Cos(a) * r;
				float z = centre[2] + Math.Sin(a) * r;
				if (OpenWater(x, z))
				{
					spot = Vector(x, WaterY(x, z), z);
					return true;
				}
			}
		}
		return false;
	}

	static bool IsWater(float x, float z)
	{
		return g_Game.SurfaceIsSea(x, z) || g_Game.SurfaceIsPond(x, z);
	}

	// the water's surface at x, z (without the waves)
	static float WaterY(float x, float z)
	{
		float bottom = g_Game.SurfaceY(x, z);
		return g_Game.GetWaterSurfaceHeightNoFakeWave(Vector(x, bottom + 100, z));
	}

	static bool OpenWater(float x, float z)
	{
		float size = g_Game.GetWorld().GetWorldSize();
		if (x < EDGE_MARGIN || z < EDGE_MARGIN || x > size - EDGE_MARGIN || z > size - EDGE_MARGIN)
			return false;
		if (!IsWater(x, z))
			return false;
		float surface = WaterY(x, z);
		for (int i = 0; i <= 8; i++)
		{
			float px = x;
			float pz = z;
			if (i > 0)
			{
				float a = Math.PI2 * i / 8;
				px = x + Math.Cos(a) * WATER_CLEAR;
				pz = z + Math.Sin(a) * WATER_CLEAR;
			}
			if (!IsWater(px, pz) || surface - g_Game.SurfaceY(px, pz) < WATER_DEPTH)
				return false;
		}
		array<Object> objects = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(Vector(x, surface, z), WATER_CLEAR, objects, proxies);
		foreach (Object obj : objects)
		{
			if (obj.IsBush() || obj.IsInherited(ItemBase) || obj.IsInherited(DayZCreature))
				continue;
			return false;
		}
		return true;
	}

	static bool AnyPlayerWithin(vector p, float radius)
	{
		array<vector> positions = PlayerPositions();
		float r2 = radius * radius;
		foreach (vector q : positions)
		{
			if (Dist2D2(p, q) <= r2)
				return true;
		}
		return false;
	}

	static bool IsPlainAscii(string s)
	{
		for (int i = 0; i < s.Length(); i++)
		{
			int c = s.Get(i).ToAscii();
			if (c < 32 || c > 126)
				return false;
		}
		return true;
	}

	// Name of the nearest town or village, for Expansion's "airdrop heading towards ..." messages.
	static string NearestTown(vector p)
	{
		string world;
		g_Game.GetWorldName(world);
		string path = "CfgWorlds " + world + " Names";
		int count = g_Game.ConfigGetChildrenCount(path);
		string best = "your position";
		float bestD = float.MAX;
		for (int i = 0; i < count; i++)
		{
			string child;
			g_Game.ConfigGetChildName(path, i, child);
			string entry = path + " " + child;
			string type = g_Game.ConfigGetTextOut(entry + " type");
			if (type != "Capital" && type != "City" && type != "Village" && type != "NameCityCapital" && type != "NameCity" && type != "NameVillage")
				continue;
			TFloatArray pos = new TFloatArray();
			g_Game.ConfigGetFloatArray(entry + " position", pos);
			if (pos.Count() < 2)
				continue;
			float dx = pos[0] - p[0];
			float dz = pos[1] - p[2];
			float d = dx * dx + dz * dz;
			if (d < bestD)
			{
				// "name" if it's plain ASCII (Nasdara: City_2 is "Dashtabad"), else the class name:
				// on Chernarus "name" is Cyrillic, but Settlement_Chernogorsk -> Chernogorsk
				string name = g_Game.ConfigGetTextOut(entry + " name");
				if (name == "" || name.IndexOf("$") == 0 || !IsPlainAscii(name))
				{
					name = child;
					int underscore = name.IndexOf("_");
					if (underscore >= 0 && underscore < name.Length() - 1)
						name = name.Substring(underscore + 1, name.Length() - underscore - 1);
					name.Replace("_", " ");
				}
				bestD = d;
				best = name;
			}
		}
		return best;
	}
}
