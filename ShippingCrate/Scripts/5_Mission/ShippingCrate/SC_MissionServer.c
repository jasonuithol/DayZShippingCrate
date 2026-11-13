modded class MissionServer
{
	// drops still standing when the server last stopped: cleared once the saved world is in
	override void OnInit()
	{
		super.OnInit();
		SC_Leftovers.NoteBefore();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Leftovers.ClearAfterRestart, SC_Leftovers.STARTUP_DELAY_MS, false);
	}

	// Every new character (first spawn, and every respawn after death) starts with a delivery radio.
	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		PlayerBase player = super.OnClientNewEvent(identity, pos, ctx);
		if (player && !player.GetInventory().CreateInInventory("SC_DropRadio"))
			Print("[ShippingCrate] no room for a delivery radio on a new character");
		return player;
	}
}
