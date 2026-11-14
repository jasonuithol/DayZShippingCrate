// The delivery radio: a knob instead of a frequency, for the vehicle (a load in
// ShippingCrate.json's "drops"). In hand, like vanilla's radio: click for the next vehicle,
// hold to order a delivery of it to where you're standing. "any" (a random vehicle) is a stop on
// the dial too, between the last and the first. The prompt shows what the knob is on, so the
// radio can be read at a glance. The guns are always a mix: the boot full of different ones.
// One delivery at a time per player: a new order waits until the last one has been cleared.
// The tuning is saved with the item as the load's name.
class SC_DropRadio: Inventory_Base
{
	// server: what the knobs are on, by name (saved)
	string m_SC_TunedVehicle;

	// synchronised: the knob's position, 1-based into the config's list, 0 = untuned ("any")
	int m_SC_VehicleKnob;

	// client: the list's names, asked for once from the server
	static ref array<string> s_VehicleNames;
	static float s_NextAsk;

	static const int RPC_ASK_NAMES = 0x5C0A11;
	static const int RPC_NAMES = 0x5C0A12;

	void SC_DropRadio()
	{
		RegisterNetSyncVariableInt("m_SC_VehicleKnob", 0, 63);
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(SC_ActionTuneVehicle);
		AddAction(SC_ActionOrderDrop);
	}

	// ---- server ----

	static array<string> VehicleNames()
	{
		array<string> names = new array<string>();
		foreach (SC_Drop d : SC_Settings.Get().drops)
			names.Insert(d.name);
		return names;
	}

	// to the next vehicle load (step 1) or the one before (-1); "any" sits between the last and
	// the first
	void SC_TuneVehicle(int step = 1)
	{
		array<string> names = VehicleNames();
		if (names.Count() == 0)
			return;
		int stops = names.Count() + 1;   // 0 is "any"
		int at = names.Find(m_SC_TunedVehicle) + 1;
		at = ((at + step) % stops + stops) % stops;
		if (at == 0)
			m_SC_TunedVehicle = "";
		else
			m_SC_TunedVehicle = names[at - 1];
		SC_SyncKnobs();
	}

	// the knob's position from the name (also after loading: the config may have changed)
	void SC_SyncKnobs()
	{
		m_SC_VehicleKnob = VehicleNames().Find(m_SC_TunedVehicle) + 1;
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

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(m_SC_TunedVehicle);
		ctx.Write("");   // where the guns knob was (0.3, 0.4): kept so older saves still load
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;
		if (!ctx.Read(m_SC_TunedVehicle))
			m_SC_TunedVehicle = "";
		string oldGunKnob;
		ctx.Read(oldGunKnob);
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
			rpc.Send(this, RPC_NAMES, true, sender);
		}
		else if (rpc_type == RPC_NAMES && !g_Game.IsDedicatedServer())
		{
			string vehicles;
			if (ctx.Read(vehicles))
			{
				s_VehicleNames = new array<string>();
				vehicles.Split(";", s_VehicleNames);
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
}

// click: the next vehicle. The prompt shows the knob: "Vehicle: Ada 4x4 (next)"
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
		m_Text = "Vehicle: " + radio.SC_VehicleLabel() + " (next)";
		return true;
	}

	override void OnExecuteServer(ActionData action_data)
	{
		SC_DropRadio radio = SC_DropRadio.Cast(action_data.m_MainItem);
		if (radio)
			radio.SC_TuneVehicle();
	}
}

class SC_ActionOrderDropCB: ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(ORDER_SECONDS);
	}

	static const float ORDER_SECONDS = 1.5;
}

// hold: order a delivery of what the knob is on, to here (vanilla's tuning animation)
class SC_ActionOrderDrop: ActionContinuousBase
{
	void SC_ActionOrderDrop()
	{
		m_CallbackClass = SC_ActionOrderDropCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ITEM_TUNE;
		m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_ITEM_TUNE;
		m_Text = "Order drop here";
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
		return SC_DropRadio.Cast(item) != null;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		SC_Order.Place(action_data.m_Player, SC_DropRadio.Cast(action_data.m_MainItem));
	}
}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(SC_ActionTuneVehicle);
		actions.Insert(SC_ActionOrderDrop);
	}
}
