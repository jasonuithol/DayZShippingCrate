// Puts the shipping container on the ground and loads it: the vehicle parked inside with its
// nose to the doors, loose items on the floor behind it.
class SC_Spawn
{
	static const float VEHICLE_LIFETIME = 3888000; // 45 days, same as vanilla vehicles
	static const float WALL = 0.25;                // keep this far from the inside of the walls
	static const float GAP = 0.3;                  // between the vehicle and the doors / the items

	// Long axis, door end and inside floor of a placed container, in its model space.
	vector m_Min;
	vector m_Max;
	int m_Axis;          // 0 = x, 2 = z
	int m_Cross;
	float m_DoorSign;    // +1 / -1: which end of m_Axis has the doors
	float m_FloorY;      // model space
	Building m_Box;

	// The container at pos (on the ground) facing yaw, doors unlocked.
	static Building PlaceContainer(int colour, vector pos, float yaw, bool openDoors)
	{
		string type = SC_Settings.ContainerType(colour);
		Building box = Building.Cast(g_Game.CreateObjectEx(type, pos, ECE_CREATEPHYSICS | ECE_UPDATEPATHGRAPH | ECE_NOLIFETIME));
		if (!box)
		{
			Print("[ShippingCrate] could not create " + type);
			return null;
		}
		box.SetOrientation(Vector(yaw, 0, 0));
		vector mm[2];
		Bounds(box, mm);
		vector at = pos;
		at[1] = g_Game.SurfaceY(pos[0], pos[2]) - mm[0][1];
		box.SetPosition(at);

		for (int i = 0; i < box.GetDoorCount(); i++)
		{
			box.UnlockDoor(i, false);
			if (openDoors)
				box.OpenDoor(i);
		}
		return box;
	}

	static void Bounds(Object obj, out vector mm[2])
	{
		if (!obj.GetCollisionBox(mm))
			obj.ClippingInfo(mm);
	}

	// Loads the container. Returns how many things were created (vehicle + items, not counting
	// what is on and in them).
	static int Load(Building box, SC_Drop drop)
	{
		if (!box || !drop)
			return 0;
		SC_Spawn s = new SC_Spawn();
		s.Measure(box);
		return s.LoadDrop(drop);
	}

	void Measure(Building box)
	{
		m_Box = box;
		vector mm[2];
		Bounds(box, mm);
		m_Min = mm[0];
		m_Max = mm[1];
		m_Axis = 2;
		m_Cross = 0;
		if (m_Max[0] - m_Min[0] > m_Max[2] - m_Min[2])
		{
			m_Axis = 0;
			m_Cross = 2;
		}
		vector centre = (m_Min + m_Max) * 0.5;

		m_DoorSign = 1;
		if (box.GetDoorCount() > 0)
		{
			vector door = box.WorldToModel(box.GetDoorSoundPos(0));
			if (door[m_Axis] < centre[m_Axis])
				m_DoorSign = -1;
		}

		// the floor: straight down through the middle until we hit the container itself
		m_FloorY = m_Min[1] + 0.15;
		vector top = box.ModelToWorld(Vector(centre[0], m_Max[1] - 0.3, centre[2]));
		vector bottom = box.ModelToWorld(Vector(centre[0], m_Min[1] - 0.5, centre[2]));
		vector hit;
		vector normal;
		int component;
		set<Object> results = new set<Object>();
		if (DayZPhysics.RaycastRV(top, bottom, hit, normal, component, results, null, null, false, false, ObjIntersectGeom))
		{
			foreach (Object o : results)
			{
				if (o == box)
				{
					m_FloorY = box.WorldToModel(hit)[1];
					break;
				}
			}
		}
		Print("[ShippingCrate] container " + box.GetType() + " size " + (m_Max - m_Min) + " long axis " + m_Axis + " doors at " + m_DoorSign + " floor " + (m_FloorY - m_Min[1]) + " m up");
	}

	// a point inside, in model space: along = distance from the middle towards the doors,
	// across = sideways from the middle
	vector Inside(float along, float across, float up)
	{
		vector centre = (m_Min + m_Max) * 0.5;
		vector p = centre;
		p[m_Axis] = centre[m_Axis] + along * m_DoorSign;
		p[m_Cross] = centre[m_Cross] + across;
		p[1] = m_FloorY + up;
		return p;
	}

	float HalfLength()
	{
		return (m_Max[m_Axis] - m_Min[m_Axis]) * 0.5 - WALL;
	}

	float HalfWidth()
	{
		return (m_Max[m_Cross] - m_Min[m_Cross]) * 0.5 - WALL;
	}

	int LoadDrop(SC_Drop drop)
	{
		int created = 0;
		float itemsFrom = -HalfLength();   // the back wall
		float itemsTo = HalfLength();      // the doors, unless a vehicle is parked there

		if (drop.vehicle && drop.vehicle.type != "")
		{
			EntityAI vehicle = Vehicle(drop.vehicle);
			if (vehicle)
			{
				created++;
				itemsTo = HalfLength() - GAP - VehicleLength(vehicle) - GAP;
			}
		}

		// loose items in rows across the floor, from the back wall towards the vehicle
		array<string> types = new array<string>();
		array<SC_Item> specs = new array<SC_Item>();
		foreach (SC_Item spec : drop.items)
		{
			for (int n = 0; n < SC_Settings.Count(spec); n++)
			{
				types.Insert(SC_Settings.Choose(spec.type));
				specs.Insert(spec);
			}
		}
		if (types.Count() == 0)
			return created;

		float length = Math.Max(itemsTo - itemsFrom, 0.5);
		float width = 2 * HalfWidth();
		int perRow = Math.Max(1, Math.Min(3, types.Count()));
		int rows = (types.Count() + perRow - 1) / perRow;
		for (int i = 0; i < types.Count(); i++)
		{
			int row = i / perRow;
			int col = i % perRow;
			float along = itemsFrom + length * (row + 0.5) / rows;
			float across = -HalfWidth() + width * (col + 0.5) / perRow;
			EntityAI item = FloorItem(types[i], Inside(along, across, 0.05));
			if (!item)
				continue;
			Fill(item, specs[i]);
			created++;
		}
		return created;
	}

	float VehicleLength(EntityAI vehicle)
	{
		vector mm[2];
		Bounds(vehicle, mm);
		return mm[1][2] - mm[0][2];
	}

	// Parked inside with its front bumper GAP short of the doors, facing out.
	EntityAI Vehicle(SC_Item spec)
	{
		string type = SC_Settings.Choose(spec.type);
		vector outward = vector.Zero;
		outward[m_Axis] = m_DoorSign;
		vector dir = m_Box.VectorToParent(outward);
		float yaw = dir.VectorToAngles()[0];

		EntityAI vehicle = EntityAI.Cast(g_Game.CreateObjectEx(type, m_Box.ModelToWorld(Inside(0, 0, 1)), ECE_CREATEPHYSICS | ECE_UPDATEPATHGRAPH));
		if (!vehicle)
		{
			Print("[ShippingCrate] could not create vehicle " + type);
			return null;
		}
		vehicle.SetOrientation(Vector(yaw, 0, 0));

		// fit it out first: it sits differently with wheels on
		Fill(vehicle, spec, true);

		vector mm[2];
		Bounds(vehicle, mm);
		float half = (mm[1][2] - mm[0][2]) * 0.5;
		float along = HalfLength() - GAP - half;
		vector p = m_Box.ModelToWorld(Inside(along, 0, 0));
		// model z runs front to back; centre it on its footprint, wheels just above the floor
		vector offset = vehicle.VectorToParent(Vector((mm[0][0] + mm[1][0]) * 0.5, 0, (mm[0][2] + mm[1][2]) * 0.5));
		p = p - offset;
		p[1] = m_Box.ModelToWorld(Inside(along, 0, 0))[1] - mm[0][1] + 0.1;
		vehicle.SetPosition(p);
		vehicle.SetOrientation(Vector(yaw, 0, 0));
		vehicle.SetLifetime(VEHICLE_LIFETIME);
		dBodyActive(vehicle, ActiveState.ACTIVE);
		return vehicle;
	}

	EntityAI FloorItem(string type, vector modelPos)
	{
		vector p = m_Box.ModelToWorld(modelPos);
		EntityAI item = EntityAI.Cast(g_Game.CreateObjectEx(type, p, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS));
		if (!item)
		{
			Print("[ShippingCrate] could not create " + type);
			return null;
		}
		vector mm[2];
		Bounds(item, mm);
		p[1] = p[1] - mm[0][1];
		item.SetPosition(p);
		item.SetOrientation(Vector(m_Box.GetOrientation()[0] + Math.RandomFloat(-20, 20), 0, 0));
		return item;
	}

	// Properties, attachments and cargo of one spawned thing, recursively. Vehicles take their
	// parts with CreateInInventory, which finds the free slot (four wheels and the spare).
	static void Fill(EntityAI item, SC_Item spec, bool isVehicle = false)
	{
		foreach (SC_Item a : spec.attachments)
		{
			for (int i = 0; i < SC_Settings.Count(a); i++)
			{
				string at = SC_Settings.Choose(a.type);
				EntityAI att;
				if (isVehicle)
					att = item.GetInventory().CreateInInventory(at);
				else
					att = item.GetInventory().CreateAttachment(at);
				if (!att)
				{
					Print("[ShippingCrate] " + item.GetType() + ": could not attach " + at);
					continue;
				}
				Fill(att, a);
			}
		}

		foreach (SC_Item c : spec.cargo)
		{
			for (int j = 0; j < SC_Settings.Count(c); j++)
			{
				string ct = SC_Settings.Choose(c.type);
				EntityAI thing = item.GetInventory().CreateEntityInCargo(ct);
				if (!thing)
				{
					Print("[ShippingCrate] " + item.GetType() + ": no room for " + ct);
					continue;
				}
				Fill(thing, c);
			}
		}

		Properties(item, spec);
	}

	static void Properties(EntityAI item, SC_Item spec)
	{
		ItemBase ib;
		if (Class.CastTo(ib, item))
		{
			if (spec.liquid != "")
			{
				int liquid = SC_Settings.LiquidType(spec.liquid);
				if (liquid == LIQUID_NONE)
					Print("[ShippingCrate] " + item.GetType() + ": unknown liquid " + spec.liquid);
				else
				{
					ib.SetLiquidType(liquid);
					if (spec.quantity <= 0)
						ib.SetQuantityMax();
				}
			}
			if (spec.quantity > 0 && ib.HasQuantity())
				ib.SetQuantityNormalized(Math.Clamp(spec.quantity, 0, 1));
		}

		Weapon_Base weapon;
		if (spec.magazine != "" && Class.CastTo(weapon, item))
		{
			string mag = SC_Settings.Choose(spec.magazine);
			if (!weapon.SpawnAttachedMagazine(mag))
				Print("[ShippingCrate] " + item.GetType() + ": could not load " + mag);
		}

		CarScript car;
		if (spec.fluids && Class.CastTo(car, item))
		{
			car.Fill(CarFluid.FUEL, car.GetFluidCapacity(CarFluid.FUEL));
			car.Fill(CarFluid.OIL, car.GetFluidCapacity(CarFluid.OIL));
			car.Fill(CarFluid.COOLANT, car.GetFluidCapacity(CarFluid.COOLANT));
			car.Fill(CarFluid.BRAKE, car.GetFluidCapacity(CarFluid.BRAKE));
		}
	}
}
