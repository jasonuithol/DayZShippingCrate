class CfgPatches
{
	class ShippingCrate
	{
		units[] = {"ShippingCrate_Airdrop", "SC_DropRadio", "SC_RunwayLight", "SC_Prop_IED", "SC_Prop_Bear", "SC_Prop_GlowRed", "SC_Prop_GlowGreen", "SC_Prop_GlowWhite", "SC_Prop_EyeRed"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Structures_Industrial", "DZ_Structures_Specific", "DZ_Sounds_Effects", "DZ_Gear_Containers", "DZ_Gear_Consumables", "DZ_Weapons_Explosives", "DZ_Radio", "DZ_Gear_Food", "DayZExpansion_Missions_Scripts", "DayZExpansion_Objects_Missions"};
	};
};

class CfgMods
{
	class ShippingCrate
	{
		dir = "ShippingCrate";
		name = "Shipping Crate";
		type = "mod";
		dependencies[] = {"World", "Mission"};

		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"ShippingCrate/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"ShippingCrate/Scripts/5_Mission"};
			};
		};
	};
};

class CfgVehicles
{
	class HouseNoDestruct;
	class Inventory_Base;

	// The delivery radio: vanilla's walkie-talkie, bright orange, and not a radio. In hand it
	// tunes to one of the loads in ShippingCrate.json; a drop on a group that has one brings
	// what it's tuned to. Every new character starts with one. The model has no hidden
	// selections to retexture, so this is a copy of it whose colour texture is solid orange.
	class SC_DropRadio: Inventory_Base
	{
		scope = 2;
		displayName = "Delivery radio";
		descriptionShort = "Bright orange and no good for talking. Tune it to the delivery you want: the next airdrop on your group brings it.";
		model = "\ShippingCrate\data\sc_radio.p3d";   // vanilla's WalkieTalkie.p3d, its texture solid orange (tools/make_radio.py)
		weight = 250;
		itemSize[] = {1, 3};
		inventorySlot[] = {"WalkieTalkie", "Chemlight"};
		rotationFlags = 1;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 100;
					healthLevels[] = {{1.0, {"DZ\gear\radio\data\WalkieTalkie.rvmat"}}, {0.7, {"DZ\gear\radio\data\WalkieTalkie.rvmat"}}, {0.5, {"DZ\gear\radio\data\WalkieTalkie_damage.rvmat"}}, {0.3, {"DZ\gear\radio\data\WalkieTalkie_damage.rvmat"}}, {0.0, {"DZ\gear\radio\data\WalkieTalkie_destruct.rvmat"}}};
				};
			};
		};
	};
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
	// The airfield's runway edge light, with its dome ("zluty pozicni", the yellow position
	// light) in semi-transparent orange that glows from within. Clients stand one on each end of
	// the container, its lamp the rotating beacon.
	class SC_RunwayLight: HouseNoDestruct
	{
		scope = 1;
		model = "DZ\structures\Specific\Airfields\Airfield_Light_Edge.p3d";
		hiddenSelections[] = {"zluty pozicni"};
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(1,0.45,0.05,0.55,CA)"};
		hiddenSelectionsMaterials[] = {"ShippingCrate\data\sc_lens_orange.rvmat"};
	};
	// Props for the roof and the plane: vanilla models and textures as buildings rather than
	// items, so players can't pick them up or do anything else with them. Clients create them
	// locally, without collision.
	class SC_Prop_IED: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\weapons\explosives\ImprovisedExplosive.p3d";
		hiddenSelections[] = {"zbytek"};
		hiddenSelectionsTextures[] = {"dz\weapons\explosives\data\ied_co.paa"};
		hiddenSelectionsMaterials[] = {"dz\weapons\explosives\data\ied.rvmat"};
	};
	class SC_Prop_Bear: HouseNoDestruct
	{
		scope = 1;
		model = "\dz\gear\containers\teddybear.p3d";
		hiddenSelections[] = {"camoGround"};
		hiddenSelectionsTextures[] = {"\dz\gear\containers\Data\teddybear_brown_co.paa"};
		hiddenSelectionsMaterials[] = {"DZ\gear\containers\data\teddybear.rvmat"};
	};
	// a chemlight's shape in a solid, strongly glowing colour: the plane's navigation markers and
	// the bear's eyes
	class SC_Prop_GlowRed: HouseNoDestruct
	{
		scope = 1;
		model = "\dz\gear\consumables\Chemlight.p3d";
		hiddenSelections[] = {"camo"};
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(1,0,0,1,CO)"};
		hiddenSelectionsMaterials[] = {"ShippingCrate\data\sc_nav_red.rvmat"};
	};
	// the teddy bear's eyes: round (a plum, every cooking state's surface in the same gentle
	// red glow), much gentler than the navigation markers
	class SC_Prop_EyeRed: HouseNoDestruct
	{
		scope = 1;
		model = "\dz\gear\food\plum.p3d";
		hiddenSelections[] = {"cs_raw", "cs_baked", "cs_boiled", "cs_dried", "cs_burned"};
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(1,0,0,1,CO)", "#(argb,8,8,3)color(1,0,0,1,CO)", "#(argb,8,8,3)color(1,0,0,1,CO)", "#(argb,8,8,3)color(1,0,0,1,CO)", "#(argb,8,8,3)color(1,0,0,1,CO)"};
		hiddenSelectionsMaterials[] = {"ShippingCrate\data\sc_eye_red.rvmat", "ShippingCrate\data\sc_eye_red.rvmat", "ShippingCrate\data\sc_eye_red.rvmat", "ShippingCrate\data\sc_eye_red.rvmat", "ShippingCrate\data\sc_eye_red.rvmat"};
	};
	class SC_Prop_GlowGreen: SC_Prop_GlowRed
	{
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(0,1,0.2,1,CO)"};
		hiddenSelectionsMaterials[] = {"ShippingCrate\data\sc_nav_green.rvmat"};
	};
	class SC_Prop_GlowWhite: SC_Prop_GlowRed
	{
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(1,1,1,1,CO)"};
		hiddenSelectionsMaterials[] = {"ShippingCrate\data\sc_nav_white.rvmat"};
	};
	// only used if Expansion is built with EXPANSION_MISSIONS_CONTAINER_CLIENT
	class ShippingCrate_Airdrop_Client: ExpansionAirdropContainerBase_Client
	{
		scope = 1;
	};
};

// the teddy bear's warning, every 3 s for the last minute before the container blows
class CfgSoundShaders
{
	class baseCharacter_SoundShader;
	class SC_Evacuate_SoundShader: baseCharacter_SoundShader
	{
		samples[] = {{"ShippingCrate\data\sounds\evacuate", 1}};
		volume = 1;
		range = 150;
	};
};

class CfgSoundSets
{
	class baseCharacter_SoundSet;
	class SC_Evacuate_SoundSet: baseCharacter_SoundSet
	{
		soundShaders[] = {"SC_Evacuate_SoundShader"};
	};
};
