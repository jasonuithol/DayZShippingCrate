// Every new character (first spawn, and every respawn after death) starts with a delivery radio.
modded class MissionServer
{
	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		PlayerBase player = super.OnClientNewEvent(identity, pos, ctx);
		if (player && !player.GetInventory().CreateInInventory("SC_DropRadio"))
			Print("[ShippingCrate] no room for a delivery radio on a new character");
		return player;
	}
}
