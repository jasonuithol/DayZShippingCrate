modded class MissionServer
{
	// drops still standing when the server last stopped: cleared once the saved world is in
	override void OnInit()
	{
		super.OnInit();
		SC_Leftovers.NoteBefore();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SC_Leftovers.ClearAfterRestart, SC_Leftovers.STARTUP_DELAY_MS, false);
	}

	// Every player always has a delivery radio: each new character (first spawn, and every
	// respawn after death) and each connect, should it have been lost (SC_DropRadio.Ensure).
	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		PlayerBase player = super.OnClientNewEvent(identity, pos, ctx);
		SC_DropRadio.Ensure(player, "new character");
		return player;
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		SC_DropRadio.Ensure(player, "joined");
	}
}
