// What a server restart would otherwise leave behind. The carrier (container or pallet deck),
// the IEDs and the bear only exist while the server runs, but the vehicles and items they
// held are saved like any others: a drop that landed and hadn't blown up yet when the server
// stopped came back as a truck, a cart, a chest and a barrel sitting on bare ground, forever.
//
// So every landed drop that will self-destruct is written down (its frame and bounds, in
// $profile:ShippingCrate/landed.json) and crossed off once it has been removed. When the
// server starts, whatever is still listed is cleared away: anything still lying where the
// carrier was, the way the self-destruct would have.
//
// A pallet's vehicle exists (and is saved) from the moment the plane drops it, so it's
// written down too, by its persistent ID, while it rides down; once the pallet has landed the
// pallet's own entry covers it. After a restart it comes back wherever it was (and, with its
// gravity back on, falls): it's found by that ID within RIDE_SEARCH of the drop.
class SC_LandedDrop
{
	string what;
	vector across;   // the carrier's frame: rows of its transform
	vector up;
	vector along;
	vector origin;
	vector min;      // its bounds in that frame
	vector max;
	bool riding;     // a vehicle still riding down: origin is the drop point, id its persistent ID
	string vehicle;
	int id1;
	int id2;
	int id3;
	int id4;
}

class SC_LandedList
{
	ref array<ref SC_LandedDrop> drops = new array<ref SC_LandedDrop>();
}

class SC_Leftovers
{
	static const string FILE = "$profile:ShippingCrate/landed.json";
	static const int STARTUP_DELAY_MS = 20000;   // after the mission starts: the saved world is in by then
	static const float RIDE_SEARCH = 700;        // metres around the drop point a left-over ride can be (it fell from 450 m)

	protected static ref SC_LandedList s_List;

	protected static SC_LandedList List()
	{
		if (!s_List)
		{
			s_List = new SC_LandedList();
			if (FileExist(FILE))
			{
				string error;
				SC_LandedList loaded;
				if (JsonFileLoader<SC_LandedList>.LoadFile(FILE, loaded, error) && loaded && loaded.drops)
					s_List = loaded;
				else
					Print("[ShippingCrate] can't read " + FILE + ": " + error);
			}
		}
		return s_List;
	}

	protected static void Save()
	{
		if (!FileExist(SC_Settings.DIR))
			MakeDirectory(SC_Settings.DIR);
		string error;
		if (!JsonFileLoader<SC_LandedList>.SaveFile(FILE, List(), error))
			Print("[ShippingCrate] can't write " + FILE + ": " + error);
	}

	// a drop has landed (frame f, bounds mn/mx in it)
	static void Remember(string what, vector f[4], vector mn, vector mx)
	{
		SC_LandedDrop d = new SC_LandedDrop();
		d.what = what;
		d.across = f[0];
		d.up = f[1];
		d.along = f[2];
		d.origin = f[3];
		d.min = mn;
		d.max = mx;
		List().drops.Insert(d);
		Save();
	}

	// ... and has been removed
	static void Forget(vector origin)
	{
		array<ref SC_LandedDrop> drops = List().drops;
		for (int i = drops.Count() - 1; i >= 0; i--)
		{
			if (!drops[i].riding && vector.Distance(drops[i].origin, origin) < 0.5)
				drops.Remove(i);
		}
		Save();
	}

	// a pallet's vehicle has set off to ride down onto dropPoint
	static void RememberRide(string what, vector dropPoint, EntityAI ride)
	{
		SC_LandedDrop d = new SC_LandedDrop();
		d.what = what;
		d.riding = true;
		d.origin = dropPoint;
		d.vehicle = ride.GetType();
		ride.GetPersistentID(d.id1, d.id2, d.id3, d.id4);
		List().drops.Insert(d);
		Save();
	}

	// ... and has landed (or gone with its crate)
	static void ForgetRide(EntityAI ride)
	{
		if (!ride)
			return;
		int b1;
		int b2;
		int b3;
		int b4;
		ride.GetPersistentID(b1, b2, b3, b4);
		array<ref SC_LandedDrop> drops = List().drops;
		for (int i = drops.Count() - 1; i >= 0; i--)
		{
			SC_LandedDrop d = drops[i];
			if (d.riding && d.id1 == b1 && d.id2 == b2 && d.id3 == b3 && d.id4 == b4 && d.vehicle == ride.GetType())
				drops.Remove(i);
		}
		Save();
	}

	// the vehicle that was riding down in d, wherever it is now; null if it's gone
	static EntityAI FindRide(SC_LandedDrop d)
	{
		array<Object> objects = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition3D(d.origin, RIDE_SEARCH, objects, proxies);
		foreach (Object obj : objects)
		{
			EntityAI e = EntityAI.Cast(obj);
			if (!e || e.GetType() != d.vehicle)
				continue;
			int b1;
			int b2;
			int b3;
			int b4;
			e.GetPersistentID(b1, b2, b3, b4);
			if (b1 == d.id1 && b2 == d.id2 && b3 == d.id3 && b4 == d.id4)
				return e;
		}
		return null;
	}

	// Delete every vehicle and item lying in the box mn..mx of frame f (a margin around it),
	// not people or animals, nor anything in skip. Things inside them go with them.
	static int ClearBox(vector f[4], vector mn, vector mx, array<Object> skip)
	{
		vector centreLocal = (mn + mx) * 0.5;
		vector centre = f[0] * centreLocal[0] + f[1] * centreLocal[1] + f[2] * centreLocal[2] + f[3];
		array<Object> objects = new array<Object>();
		array<CargoBase> proxies = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(centre, vector.Distance(mn, mx) * 0.5 + 1, objects, proxies);
		int removed = 0;
		foreach (Object obj : objects)
		{
			if (!obj || (skip && skip.Find(obj) >= 0) || obj.IsMan() || obj.IsInherited(DayZCreature) || !obj.IsInherited(EntityAI))
				continue;
			if (obj.IsInherited(ShippingCrate_Airdrop))
				continue;
			EntityAI e = EntityAI.Cast(obj);
			if (e.GetHierarchyParent())
				continue;
			vector d = obj.GetPosition() - f[3];
			vector rel = Vector(vector.Dot(d, f[0]), vector.Dot(d, f[1]), vector.Dot(d, f[2]));
			if (rel[0] < mn[0] - 0.2 || rel[0] > mx[0] + 0.2 || rel[1] < mn[1] - 0.5 || rel[1] > mx[1] + 0.5 || rel[2] < mn[2] - 0.2 || rel[2] > mx[2] + 0.2)
				continue;
			g_Game.ObjectDelete(obj);
			removed++;
		}
		return removed;
	}

	// The entries from before the restart, noted as the server starts: the clean-up comes a
	// little later and mustn't touch a drop that has set off since. They stay in the file
	// until they're cleared, in case the server stops again first.
	protected static ref array<ref SC_LandedDrop> s_Before;

	static void NoteBefore()
	{
		s_Before = new array<ref SC_LandedDrop>();
		foreach (SC_LandedDrop d : List().drops)
			s_Before.Insert(d);
	}

	// server start (STARTUP_DELAY_MS after NoteBefore): clear away the drops that were still
	// standing when it last stopped
	static void ClearAfterRestart()
	{
		if (!s_Before)
			return;
		Clear(s_Before);
		s_Before = null;
	}

	// every listed drop, now (the test)
	static void ClearAll()
	{
		array<ref SC_LandedDrop> all = new array<ref SC_LandedDrop>();
		foreach (SC_LandedDrop d : List().drops)
			all.Insert(d);
		Clear(all);
	}

	protected static void Clear(array<ref SC_LandedDrop> drops)
	{
		if (drops.Count() == 0)
			return;
		foreach (SC_LandedDrop d : drops)
		{
			if (d.riding)
			{
				EntityAI ride = FindRide(d);
				if (ride)
					g_Game.ObjectDelete(ride);
				Print("[ShippingCrate] left over from before the restart: " + d.what + " still riding down to " + d.origin + ", " + d.vehicle + " removed: " + (ride != null));
				continue;
			}
			vector f[4];
			f[0] = d.across;
			f[1] = d.up;
			f[2] = d.along;
			f[3] = d.origin;
			int removed = ClearBox(f, d.min, d.max, null);
			Print("[ShippingCrate] left over from before the restart: " + d.what + " at " + d.origin + ", removed " + removed + " things");
		}
		foreach (SC_LandedDrop gone : drops)
			List().drops.RemoveItem(gone);
		Save();
	}
}
