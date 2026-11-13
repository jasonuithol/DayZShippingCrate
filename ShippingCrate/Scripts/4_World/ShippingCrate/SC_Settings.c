// Shipping Crate settings: what the container holds, and how the drop spot is chosen. Data, in
// $profile:ShippingCrate/ShippingCrate.json; the server writes the defaults there on its first
// start. See README.md for every field.

// One thing to spawn, and everything on and in it. A vehicle is one of these too.
class SC_Item
{
	string type;                 // class name; "A|B|C" picks one at random
	int count;                   // how many of it; left out (0) means 1
	float quantity;              // 0..1 of its maximum (stack, liquid, ammo); left out (0) leaves the default
	string liquid;               // bottles, canisters, pots: Water, Gasoline, Diesel, Vodka, Beer...
	string magazine;             // firearms: a magazine loaded into it, with a round chambered
	bool fluids;                 // vehicles: fill fuel, oil, coolant and brake fluid
	[NonSerialized()]
	bool ifRoom;                 // made in script: no complaint when it doesn't fit (a boot filled up)
	ref array<ref SC_Item> attachments = new array<ref SC_Item>();
	ref array<ref SC_Item> cargo = new array<ref SC_Item>();
}

// One possible load: a vehicle and loose items, in the shipping container (vehicle nose to the
// doors, items behind it) or, for a vehicle too big for it, on a pallet (vehicle at one end,
// items on the free end).
class SC_Drop
{
	string name;
	float weight = 1;            // how often this load comes up relative to the others
	ref SC_Item vehicle;         // optional
	string carrier;              // "container", "pallet", or left out: whichever the vehicle fits
	ref array<ref SC_Item> items = new array<ref SC_Item>();
}

// A gun kit: what goes in the vehicle's boot (or on the floor if the load has no vehicle). Picked
// separately from the vehicle: the delivery radio has a knob for each.
class SC_Gun
{
	string name;
	float weight = 1;
	ref array<ref SC_Item> cargo = new array<ref SC_Item>();
	[NonSerialized()]
	int floorCount;              // made in script: only the first this many go on a floor (0: all)
}

class SC_Config
{
	bool enabled = true;
	float clusterRadius = 1000;  // players this close to each other count as one group
	float dropRadius = 1;        // Expansion's random spread around the drop spot (wind drift grows with it)
	float searchRadius = 200;    // how far from the group's centre to look for flat, clear ground
	float maxSlope = 8;          // degrees
	bool openDoors = false;      // the doors are always unlocked; this opens them too
	int selfDestructMinutes = 10; // after landing: then the bear warns, then it all blows up and is removed; -1 = never
	int warningSeconds = 60;     // how long the bear warns before it blows
	float fallSpeedScale = 1;    // the parachute's fall speed, times Expansion's
	ref array<string> colours = {"Blue", "Yellow", "Orange", "Red"};
	ref array<ref SC_Drop> drops = new array<ref SC_Drop>();   // the vehicle loads
	ref array<ref SC_Gun> guns = new array<ref SC_Gun>();
}

class SC_Settings
{
	static const string DIR = "$profile:ShippingCrate";
	static const string FILE = "$profile:ShippingCrate/ShippingCrate.json";
	static const string DEFAULTS = "ShippingCrate/ShippingCrate.json"; // packed in the mod

	// The four vanilla locked containers (1.23+), the ones with doors. Index + 1 is what the
	// airdrop crate synchronises to clients, so this list is fixed; the config picks from it.
	static ref array<string> COLOURS = {"Blue", "Yellow", "Orange", "Red"};

	protected static ref SC_Config s_Config;

	static SC_Config Get()
	{
		if (!s_Config)
			Load();
		return s_Config;
	}

	// Loads the settings, writing the defaults out first if the server has none yet.
	static void Load()
	{
		string error;
		if (!FileExist(FILE))
		{
			SC_Config defaults;
			if (!JsonFileLoader<SC_Config>.LoadFile(DEFAULTS, defaults, error))
			{
				Print("[ShippingCrate] cannot read the default settings: " + error);
				s_Config = new SC_Config();
				s_Config.enabled = false;
				return;
			}
			MakeDirectory(DIR);
			if (JsonFileLoader<SC_Config>.SaveFile(FILE, defaults, error))
				Print("[ShippingCrate] wrote the default settings to " + FILE);
			else
				Print("[ShippingCrate] cannot write " + FILE + ": " + error);
			s_Config = defaults;
		}
		else
		{
			SC_Config loaded;
			if (!JsonFileLoader<SC_Config>.LoadFile(FILE, loaded, error))
			{
				Print("[ShippingCrate] " + FILE + ": " + error + " - airdrops stay vanilla Expansion");
				s_Config = new SC_Config();
				s_Config.enabled = false;
				return;
			}
			s_Config = loaded;
		}
		Sanitise(s_Config);
		Print("[ShippingCrate] " + s_Config.drops.Count() + " loads, enabled: " + s_Config.enabled);
	}

	// The JSON loader leaves a field it doesn't find at 0, not at its default in the class, so
	// anything left out of the file is 0 here: these put the real defaults back.
	static int Count(SC_Item spec)
	{
		if (spec.count <= 0)
			return 1;
		return spec.count;
	}

	protected static void Sanitise(SC_Config c)
	{
		if (c.clusterRadius <= 0)
			c.clusterRadius = 1000;
		if (c.searchRadius < 0)
			c.searchRadius = 200;
		if (c.maxSlope <= 0)
			c.maxSlope = 8;
		if (c.dropRadius < 0)
			c.dropRadius = 0;
		if (c.selfDestructMinutes == 0)
			c.selfDestructMinutes = 10;
		if (c.warningSeconds <= 0)
			c.warningSeconds = 60;
		if (c.fallSpeedScale <= 0)
			c.fallSpeedScale = 1;
		if (!c.colours || c.colours.Count() == 0)
			c.colours = {"Blue", "Yellow", "Orange", "Red"};
		if (!c.drops)
			c.drops = new array<ref SC_Drop>();
		if (!c.guns)
			c.guns = new array<ref SC_Gun>();
		foreach (SC_Gun g : c.guns)
		{
			if (g.weight <= 0)
				g.weight = 1;
			if (!g.cargo)
				g.cargo = new array<ref SC_Item>();
			foreach (SC_Item gi : g.cargo)
				SanitiseItem(gi);
		}
		foreach (SC_Drop d : c.drops)
		{
			if (d.weight <= 0)
				d.weight = 1;
			if (!d.items)
				d.items = new array<ref SC_Item>();
			if (d.vehicle)
				SanitiseItem(d.vehicle);
			foreach (SC_Item i : d.items)
				SanitiseItem(i);
		}
	}

	protected static void SanitiseItem(SC_Item i)
	{
		if (!i.attachments)
			i.attachments = new array<ref SC_Item>();
		if (!i.cargo)
			i.cargo = new array<ref SC_Item>();
		foreach (SC_Item a : i.attachments)
			SanitiseItem(a);
		foreach (SC_Item c : i.cargo)
			SanitiseItem(c);
	}

	// weighted random choice of load
	static SC_Drop Pick()
	{
		array<ref SC_Drop> drops = Get().drops;
		float total = 0;
		foreach (SC_Drop d : drops)
			total += d.weight;
		if (total <= 0)
			return null;

		float r = Math.RandomFloat(0, total);
		foreach (SC_Drop drop : drops)
		{
			r -= drop.weight;
			if (r <= 0)
				return drop;
		}
		return drops[drops.Count() - 1];
	}

	// weighted random gun kit; null if there are none
	static SC_Gun PickGun()
	{
		array<ref SC_Gun> guns = Get().guns;
		float total = 0;
		foreach (SC_Gun g : guns)
			total += g.weight;
		if (total <= 0)
			return null;
		float r = Math.RandomFloat(0, total);
		foreach (SC_Gun gun : guns)
		{
			r -= gun.weight;
			if (r <= 0)
				return gun;
		}
		return guns[guns.Count() - 1];
	}

	// The guns knob on "any": one of every kit's gun (loaded) with a quarter of its spare
	// ammunition, in random order, the round repeated MIXED_ROUNDS times so it fills the boot
	// (what doesn't fit is left out). Without a boot only the first round goes on the floor.
	static const int MIXED_ROUNDS = 4;

	static SC_Gun MixedGuns()
	{
		array<ref SC_Gun> kits = Get().guns;
		if (kits.Count() == 0)
			return null;
		SC_Gun mixed = new SC_Gun();
		mixed.name = "a mix of guns";
		for (int round = 0; round < MIXED_ROUNDS; round++)
		{
			array<int> order = new array<int>();
			for (int k = 0; k < kits.Count(); k++)
				order.Insert(k);
			order.ShuffleArray();
			foreach (int i : order)
			{
				SC_Gun kit = kits[i];
				if (kit.cargo.Count() == 0)
					continue;
				mixed.cargo.Insert(OneOf(kit.cargo[0], 1));
				for (int c = 1; c < kit.cargo.Count(); c++)
					mixed.cargo.Insert(OneOf(kit.cargo[c], Math.Max(1, Math.Round(Count(kit.cargo[c]) / 4.0))));
			}
			if (round == 0)
				mixed.floorCount = mixed.cargo.Count();
		}
		return mixed;
	}

	// spec with a different count (attachments and all), quiet when there's no room
	protected static SC_Item OneOf(SC_Item spec, int count)
	{
		SC_Item copy = new SC_Item();
		copy.type = spec.type;
		copy.count = count;
		copy.quantity = spec.quantity;
		copy.liquid = spec.liquid;
		copy.magazine = spec.magazine;
		copy.attachments = spec.attachments;
		copy.cargo = spec.cargo;
		copy.ifRoom = true;
		return copy;
	}

	// 1-based index into COLOURS, picked from the configured colours
	static int PickColour()
	{
		array<string> allowed = Get().colours;
		array<int> indices = new array<int>();
		foreach (string c : allowed)
		{
			for (int i = 0; i < COLOURS.Count(); i++)
			{
				string known = COLOURS[i];
				if (known == c)
					indices.Insert(i + 1);
			}
		}
		if (indices.Count() == 0)
			return 1;
		return indices.GetRandomElement();
	}

	static string ContainerType(int colour)
	{
		if (colour < 1 || colour > COLOURS.Count())
			colour = 1;
		return "Land_ContainerLocked_" + COLOURS[colour - 1] + "_DE";
	}

	// "A|B|C" -> one of them
	static string Choose(string spec)
	{
		array<string> options = new array<string>();
		spec.Split("|", options);
		if (options.Count() == 0)
			return spec;
		string choice = options.GetRandomElement();
		choice.Trim();
		return choice;
	}

	static int LiquidType(string name)
	{
		string n = name;
		n.ToLower();
		switch (n)
		{
			case "water": return LIQUID_WATER;
			case "cleanwater": return LIQUID_CLEANWATER;
			case "gasoline": return LIQUID_GASOLINE;
			case "petrol": return LIQUID_GASOLINE;
			case "diesel": return LIQUID_DIESEL;
			case "vodka": return LIQUID_VODKA;
			case "beer": return LIQUID_BEER;
			case "disinfectant": return LIQUID_DISINFECTANT;
			case "saline": return LIQUID_SALINE;
		}
		return LIQUID_NONE;
	}
}
