// The delivery radio: two knobs instead of a frequency, one for the vehicle (a load in
// ShippingCrate.json's "drops") and one for the guns in its boot (a kit in "guns"). In hand,
// like vanilla's radio: click for the next vehicle, hold to step through the guns (one a
// second). Both prompts show what the knob is on, so the radio can be read at a glance.
//
// When a drop starts, the server looks through the target group's inventories: each knob tuned
// on anyone's radio decides its half (one of them at random if several are); an untuned one
// leaves its half to chance by weight. The tuning is saved with the item as the load's and the
// kit's names.
class SC_DropRadio: Inventory_Base
{
	// server: what the knobs are on, by name (saved)
	string m_SC_TunedVehicle;
	string m_SC_TunedGun;

	// synchronised: the knobs' positions, 1-based into the config's lists, 0 = untuned
	int m_SC_VehicleKnob;
	int m_SC_GunKnob;

	// client: the lists' names, asked for once from the server
	static ref array<string> s_VehicleNames;
	static ref array<string> s_GunNames;
	static float s_NextAsk;

	static const int RPC_ASK_NAMES = 0x5C0A11;
	static const int RPC_NAMES = 0x5C0A12;

	void SC_DropRadio()
	{
		RegisterNetSyncVariableInt("m_SC_VehicleKnob", 0, 63);
		RegisterNetSyncVariableInt("m_SC_GunKnob", 0, 63);
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(SC_ActionTuneVehicle);
		AddAction(SC_ActionTuneGuns);
	}

	// ---- server ----

	static array<string> VehicleNames()
	{
		array<string> names = new array<string>();
		foreach (SC_Drop d : SC_Settings.Get().drops)
			names.Insert(d.name);
		return names;
	}

	static array<string> GunNames()
	{
		array<string> names = new array<string>();
		foreach (SC_Gun g : SC_Settings.Get().guns)
			names.Insert(g.name);
		return names;
	}

	// to the next vehicle load
	void SC_TuneVehicle()
	{
		array<string> names = VehicleNames();
		if (names.Count() == 0)
			return;
		m_SC_TunedVehicle = names[(names.Find(m_SC_TunedVehicle) + 1) % names.Count()];
		SC_SyncKnobs();
	}

	// to the next gun kit
	void SC_TuneGuns()
	{
		array<string> names = GunNames();
		if (names.Count() == 0)
			return;
		m_SC_TunedGun = names[(names.Find(m_SC_TunedGun) + 1) % names.Count()];
		SC_SyncKnobs();
	}

	// knob positions from the names (also after loading: the config may have changed)
	void SC_SyncKnobs()
	{
		m_SC_VehicleKnob = VehicleNames().Find(m_SC_TunedVehicle) + 1;
		m_SC_GunKnob = GunNames().Find(m_SC_TunedGun) + 1;
		SetSynchDirty();
	}

	// what the knobs are on, null if untuned (or the config no longer has it)
	SC_Drop SC_TunedDrop()
	{
		foreach (SC_Drop d : SC_Settings.Get().drops)
		{
			if (m_SC_TunedVehicle != "" && d.name == m_SC_TunedVehicle)
				return d;
		}
		return null;
	}

	SC_Gun SC_TunedGunKit()
	{
		foreach (SC_Gun g : SC_Settings.Get().guns)
		{
			if (m_SC_TunedGun != "" && g.name == m_SC_TunedGun)
				return g;
		}
		return null;
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(m_SC_TunedVehicle);
		ctx.Write(m_SC_TunedGun);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;
		if (!ctx.Read(m_SC_TunedVehicle))
			m_SC_TunedVehicle = "";
		if (!ctx.Read(m_SC_TunedGun))
			m_SC_TunedGun = "";
		return true;
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (g_Game.IsDedicatedServer())
			SC_SyncKnobs();
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		if (rpc_type == RPC_ASK_NAMES && g_Game.IsDedicatedServer())
		{
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(SC_Join(VehicleNames()));
			rpc.Write(SC_Join(GunNames()));
			rpc.Send(this, RPC_NAMES, true, sender);
		}
		else if (rpc_type == RPC_NAMES && !g_Game.IsDedicatedServer())
		{
			string vehicles;
			string guns;
			if (ctx.Read(vehicles) && ctx.Read(guns))
			{
				s_VehicleNames = new array<string>();
				s_GunNames = new array<string>();
				vehicles.Split(";", s_VehicleNames);
				guns.Split(";", s_GunNames);
			}
		}
	}

	static string SC_Join(array<string> names)
	{
		string joined = "";
		foreach (string n : names)
		{
			if (joined != "")
				joined += ";";
			joined += n;
		}
		return joined;
	}

	// ---- client: what the prompts say ----

	string SC_VehicleLabel()
	{
		return SC_Label(m_SC_VehicleKnob, s_VehicleNames);
	}

	string SC_GunLabel()
	{
		return SC_Label(m_SC_GunKnob, s_GunNames);
	}

	protected string SC_Label(int knob, array<string> names)
	{
		if (knob == 0)
			return "any";
		if (!names)
		{
			// ask the server for the names (again every few seconds until they come)
			if (g_Game.GetTickTime() >= s_NextAsk)
			{
				s_NextAsk = g_Game.GetTickTime() + 3;
				g_Game.RPCSingleParam(this, RPC_ASK_NAMES, new Param1<int>(0), true, null);
			}
			return "#" + knob;
		}
		if (knob > names.Count())
			return "#" + knob;
		return names[knob - 1];
	}

	// ---- picking for a drop ----

	// the radios on players (anywhere in their inventories, hands included)
	static array<SC_DropRadio> Carried(array<PlayerBase> players)
	{
		array<SC_DropRadio> radios = new array<SC_DropRadio>();
		foreach (PlayerBase player : players)
		{
			array<EntityAI> items = new array<EntityAI>();
			player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
			foreach (EntityAI item : items)
			{
				SC_DropRadio radio;
				if (Class.CastTo(radio, item))
					radios.Insert(radio);
			}
		}
		return radios;
	}

	// what one of these radios' vehicle knob is on, at random; null if none is tuned
	static SC_Drop ChooseDrop(array<SC_DropRadio> radios)
	{
		array<SC_Drop> wanted = new array<SC_Drop>();
		foreach (SC_DropRadio radio : radios)
		{
			SC_Drop d = radio.SC_TunedDrop();
			if (d)
				wanted.Insert(d);
		}
		if (wanted.Count() == 0)
			return null;
		return wanted.GetRandomElement();
	}

	// ... and the guns knob
	static SC_Gun ChooseGun(array<SC_DropRadio> radios)
	{
		array<SC_Gun> wanted = new array<SC_Gun>();
		foreach (SC_DropRadio radio : radios)
		{
			SC_Gun g = radio.SC_TunedGunKit();
			if (g)
				wanted.Insert(g);
		}
		if (wanted.Count() == 0)
			return null;
		return wanted.GetRandomElement();
	}
}

// click: the next vehicle. The prompt shows the knob: "Vehicle: Ada"
class SC_ActionTuneVehicle: ActionSingleUseBase
{
	void SC_ActionTuneVehicle()
	{
		m_Text = "Vehicle";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTNone();
	}

	override bool HasTarget()
	{
		return false;
	}

	override bool HasProneException()
	{
		return true;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		SC_DropRadio radio = SC_DropRadio.Cast(item);
		if (!radio)
			return false;
		m_Text = "Vehicle: " + radio.SC_VehicleLabel();
		return true;
	}

	override void OnExecuteServer(ActionData action_data)
	{
		SC_DropRadio radio = SC_DropRadio.Cast(action_data.m_MainItem);
		if (radio)
			radio.SC_TuneVehicle();
	}
}

class SC_ActionTuneGunsCB: ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousRepeat(1.0);
	}
}

// hold: step through the gun kits, one a second, with vanilla's tuning animation. The prompt
// shows the knob: "Guns: SVD"
class SC_ActionTuneGuns: ActionContinuousBase
{
	void SC_ActionTuneGuns()
	{
		m_CallbackClass = SC_ActionTuneGunsCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ITEM_TUNE;
		m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_ITEM_TUNE;
		m_Text = "Guns";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTNone();
	}

	override bool HasTarget()
	{
		return false;
	}

	override bool HasProneException()
	{
		return true;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		SC_DropRadio radio = SC_DropRadio.Cast(item);
		if (!radio)
			return false;
		m_Text = "Guns: " + radio.SC_GunLabel();
		return true;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		SC_DropRadio radio = SC_DropRadio.Cast(action_data.m_MainItem);
		if (radio)
			radio.SC_TuneGuns();
	}
}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(SC_ActionTuneVehicle);
		actions.Insert(SC_ActionTuneGuns);
	}
}
