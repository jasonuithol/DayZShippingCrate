// The pallet: what a vehicle too big for the container rides down on. A deck of vanilla wooden
// pallets, as wide as the vehicle plus a margin and as long as it plus END_ROOM: the vehicle at
// one end, nose out, and the drop's loose items (a hand cart, a sea chest, a barrel...) on the
// free end.
//
// Shared by the server (the real deck) and clients (the falling stand-in). The layout follows
// from the vehicle's type alone, so both sides work it out the same way. Deck model space: x
// across, z along (the vehicle's nose towards -z, the free end +z), y up from the bottom of the
// deck; the origin is the middle of the deck's bottom.
class SC_Pallet
{
	static const string TILE = "StaticObj_Misc_Pallet";
	static const float SIDE_MARGIN = 0.3;    // deck beyond the vehicle's sides
	static const float END_ROOM = 2.5;       // free deck at the +z end for the loose items
	static const float NOSE_GAP = 0.55;      // from the -z edge to the vehicle's nose (SC_Spawn's WALL + GAP)

	// what fits the 20 ft container (inside about 6.0 x 2.3 x 2.4 m), body sizes without wheels
	static const float FIT_LENGTH = 5.3;
	static const float FIT_WIDTH = 2.0;    // the body: wheels stick out past it (an M1025's 2.1 m body doesn't fit)
	static const float FIT_HEIGHT = 2.35;

	protected static ref map<string, ref array<vector>> s_Boxes = new map<string, ref array<vector>>();

	// Collision box of type, measured once on a throwaway copy. For a vehicle that's its body:
	// wheels are attachments and not part of it.
	static bool BodyBox(string type, out vector mm[2])
	{
		array<vector> known;
		if (!s_Boxes.Find(type, known))
		{
			// clients measure the bare model: a local vehicle is a crash waiting to happen
			Object probe;
			if (g_Game.IsDedicatedServer())
				probe = g_Game.CreateObjectEx(type, "100 -50 100", 0);
			else
				probe = ModelOnly(type, "100 -50 100");
			if (!probe)
			{
				Print("[ShippingCrate] can't measure " + type);
				return false;
			}
			vector pm[2];
			if (!probe.GetCollisionBox(pm))
				probe.ClippingInfo(pm);
			g_Game.ObjectDelete(probe);
			known = new array<vector>();
			known.Insert(pm[0]);
			known.Insert(pm[1]);
			s_Boxes.Insert(type, known);
		}
		mm[0] = known[0];
		mm[1] = known[1];
		return true;
	}

	// Client: type's 3D model as a plain local static object (no simulation, no inventory, no
	// physics body to lose). A real local vehicle crashes the game once its body is removed.
	static Object ModelOnly(string type, vector pos)
	{
		string model = g_Game.ConfigGetTextOut("CfgVehicles " + type + " model");
		if (model == "")
			return null;
		if (model.IndexOf("\\") == 0)
			model = model.Substring(1, model.Length() - 1);
		return g_Game.CreateStaticObjectUsingP3D(model, pos, vector.Zero, 1.0, true);
	}

	static vector Size(string type)
	{
		vector mm[2];
		if (!BodyBox(type, mm))
			return vector.Zero;
		return mm[1] - mm[0];
	}

	static bool FitsContainer(string vehicleType)
	{
		vector s = Size(vehicleType);
		return s[2] <= FIT_LENGTH && s[0] <= FIT_WIDTH && s[1] <= FIT_HEIGHT;
	}

	// the deck for vehicleType: nx across by nz along, and its bounds in deck space
	static void Layout(string vehicleType, out int nx, out int nz, out vector deck[2])
	{
		vector t = Size(TILE);
		vector s = Size(vehicleType);
		nx = Math.Max(2, Math.Ceil((s[0] + 2 * SIDE_MARGIN) / t[0]));
		nz = Math.Max(3, Math.Ceil((s[2] + NOSE_GAP + 0.3 + END_ROOM) / t[2]));
		deck[0] = Vector(-nx * t[0] * 0.5, 0, -nz * t[2] * 0.5);
		deck[1] = Vector(nx * t[0] * 0.5, t[1], nz * t[2] * 0.5);
	}

	// deck space -> world, for a deck whose frame is f (rows: across, up, along, origin)
	static vector ToWorld(vector f[4], vector p)
	{
		return f[0] * p[0] + f[1] * p[1] + f[2] * p[2] + f[3];
	}

	// The deck's frame on the ground: level, its long axis along forward (the crate's own z axis,
	// flattened), its bottom middle at pos's x/z. Taken from the crate's transform, not its yaw
	// angle, so the landed pallet lines up exactly with the falling one (converting the angle
	// back to a direction got the sense wrong twice).
	static void GroundFrame(vector pos, vector forward, out vector f[4])
	{
		vector along = Vector(forward[0], 0, forward[2]);
		if (along.Length() < 0.01)
			along = "0 0 1";
		along.Normalize();
		f[2] = along;
		f[1] = "0 1 0";
		f[0] = Vector(along[2], 0, -along[0]);   // up x along: right of along
		f[3] = Vector(pos[0], g_Game.SurfaceY(pos[0], pos[2]), pos[2]);
	}

	// Where the crate sits on the deck, along it: under the middle of the free end, clear of
	// the vehicle (a vehicle riding down overlapping the crate shoves it through the ground).
	static float CrateAlong(vector deck[2])
	{
		return deck[1][2] - END_ROOM * 0.5;
	}

	// Move frame f (whose origin is the crate's bottom, or the crate's spot on the ground) so its
	// origin is the deck's bottom middle; on the ground, keep it on the ground.
	static void FromCrate(inout vector f[4], vector deck[2], bool onGround)
	{
		vector o = f[3] - f[2] * CrateAlong(deck);
		if (onGround)
			o[1] = g_Game.SurfaceY(o[0], o[2]);
		f[3] = o;
	}

	// Transform of tile (i, j) in frame f
	static void TileTransform(vector f[4], int i, int j, vector deck[2], out vector mat[4])
	{
		vector tm[2];
		BodyBox(TILE, tm);
		vector t = tm[1] - tm[0];
		vector cell = Vector(deck[0][0] + (i + 0.5) * t[0], 0, deck[0][2] + (j + 0.5) * t[2]);
		vector bottomCentre = Vector((tm[0][0] + tm[1][0]) * 0.5, tm[0][1], (tm[0][2] + tm[1][2]) * 0.5);
		mat[0] = f[0];
		mat[1] = f[1];
		mat[2] = f[2];
		mat[3] = ToWorld(f, cell - bottomCentre);
	}

	// Transform of the vehicle sitting on the deck, nose towards -z (its own forward is +z, so
	// it's turned half round), NOSE_GAP from that edge, centred across. lift raises it off the
	// deck (the server drops it from just above so it settles).
	static void VehicleTransform(vector f[4], string vehicleType, vector deck[2], float lift, out vector mat[4])
	{
		vector vm[2];
		BodyBox(vehicleType, vm);
		vector s = vm[1] - vm[0];
		vector target = Vector(0, deck[1][1] + lift, deck[0][2] + NOSE_GAP + s[2] * 0.5);
		vector bottomCentre = Vector((vm[0][0] + vm[1][0]) * 0.5, vm[0][1], (vm[0][2] + vm[1][2]) * 0.5);
		mat[0] = -f[0];
		mat[1] = f[1];
		mat[2] = -f[2];
		// the vehicle's bottom centre, turned half round, lands on target
		mat[3] = ToWorld(f, target) - (mat[0] * bottomCentre[0] + mat[1] * bottomCentre[1] + mat[2] * bottomCentre[2]);
	}
}
