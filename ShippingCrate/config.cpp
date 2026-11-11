class CfgPatches
{
	class ShippingCrate
	{
		units[] = {"ShippingCrate_Airdrop"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Structures_Industrial", "DayZExpansion_Missions_Scripts", "DayZExpansion_Objects_Missions"};
	};
};

class CfgMods
{
	class ShippingCrate
	{
		dir = "ShippingCrate";
		name = "Shipping Crate";
		type = "mod";
		dependencies[] = {"World"};

		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"ShippingCrate/Scripts/4_World"};
			};
		};
	};
};

class CfgVehicles
{
	class ExpansionAirdropContainerBase_Server;
	class ExpansionAirdropContainerBase_Client;

	// What Expansion's plane actually drops for us: its own parachute crate, with no loot and no
	// infected. Clients draw a shipping container under two parachutes in its place (see
	// SC_Airdrop.c); on landing the server swaps it for the real container and its contents.
	class ShippingCrate_Airdrop: ExpansionAirdropContainerBase_Server
	{
		scope = 1;
		displayName = "Shipping container";
	};
	// only used if Expansion is built with EXPANSION_MISSIONS_CONTAINER_CLIENT
	class ShippingCrate_Airdrop_Client: ExpansionAirdropContainerBase_Client
	{
		scope = 1;
	};
};
