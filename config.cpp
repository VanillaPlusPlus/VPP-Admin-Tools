class CfgPatches
{
	class DZM_VPPAdminTools
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Data",
			"RPC_Scripts",
			"DZM_VPPATExtensions"
		};
	};
};
class CfgAddons
{
	class PreloadBanks{};
	class PreloadAddons
	{
		class DayZ
		{
			list[] = {"VPPNotifications","DZ_A_Airport","DZ_Animals","DZ_Animals_AnimConfig","DZ_Animals_Birds","DZ_Animals_Birds_Chicken","DZ_Animals_Birds_Cock","DZ_Animals_Birds_Crow","DZ_Animals_Birds_hawk","DZ_Animals_Birds_Seagull","DZ_Animals_Cow","DZ_Animals_Deer","DZ_Animals_Dogs_Fin","DZ_Animals_Dogs_Pastor","DZ_Animals_Goat","DZ_Animals_Insect","DZ_Animals_Rabbit","DZ_Animals_Rat","DZ_Animals_Sheep","DZ_Animals_WildBoar","DZ_Anims","DZ_Anims_Config_Sdr","DZ_Anims_Config_Wmn","DZ_Anims_Config_Zmb","DZ_Buildings","DZ_Buildings2","DZ_Buildings2_A_Crane_02","DZ_Buildings2_A_GENERALSTORE_01","DZ_Buildings2_A_Pub","DZ_Buildings2_A_Statue","DZ_Buildings2_Barn_Metal","DZ_Buildings2_BuildingParts","DZ_Buildings2_BuildingParts_Signs","DZ_Buildings2_BuildingParts_Signs_TEC","DZ_Buildings2_Church_01","DZ_Buildings2_Farm_Cowshed","DZ_Buildings2_Farm_WTower","DZ_Buildings2_HouseBlocks_HouseBlock_A","DZ_Buildings2_HouseBlocks_HouseBlock_B","DZ_Buildings2_HouseBlocks_HouseBlock_C","DZ_Buildings2_HouseBlocks_HouseBlock_D","DZ_Buildings2_Ind_CementWorks_Ind_Dopravnik","DZ_Buildings2_Ind_CementWorks_Ind_Expedice","DZ_Buildings2_Ind_CementWorks_Ind_MalyKomin","DZ_Buildings2_Ind_CementWorks_Ind_Mlyn","DZ_Buildings2_Ind_CementWorks_Ind_Pec","DZ_Buildings2_Ind_CementWorks_Ind_SiloMale","DZ_Buildings2_Ind_CementWorks_Ind_SiloVelke","DZ_Buildings2_Ind_CementWorks_Ind_Vysypka","DZ_Buildings2_Ind_Garage01","DZ_Buildings2_Ind_Pipeline_IndPipe2","DZ_Buildings2_Ind_Shed_01","DZ_Buildings2_Ind_Shed_02","DZ_Buildings2_Ind_Tank","DZ_Buildings2_Ind_Workshop01","DZ_Buildings2_Misc_Cargo","DZ_Buildings2_Misc_PowerStation","DZ_Buildings2_Misc_WaterStation","DZ_Buildings2_Rail_House_01","DZ_Buildings2_Shed_small","DZ_Buildings2_Shed_wooden","DZ_Characters","DZ_Characters_Backpacks","DZ_Characters_Glasses","DZ_Characters_Gloves","DZ_Characters_Headgear","DZ_Characters_Heads","DZ_Characters_Pants","DZ_Characters_Masks","DZ_Characters_Shoes","DZ_Characters_Tops","DZ_Characters_Vests","DZ_Code","DZ_Data","DZ_Data_Data_ParticleEffects","DZ_Functions","DZ_Gear_Crafting","DZ_Gear_Cooking","DZ_Gear_Consumables","DZ_Gear_Containers","DZ_Gear_Drinks","DZ_Gead_Food","DZ_Gear_Medical","DZ_Gear_Navigation","DZ_Gear_Optics","DZ_Gear_Tools","DZ_Gear_Traps","DZ_Items","DZ_Items_Drinks","DZ_Items_Food","DZ_Items_Medical","DZ_Items_Misc","DZ_Items_Trash","DZ_Items_Weapons","DZ_Misc","DZ_Misc2","DZ_Misc2_Samsite","DZ_Misc3","DZ_Modules","DZ_Roads2","DZ_Roads2_Bridge","DZ_Roads2_Dam","DZ_Rocks2","DZ_Rocks3","DZ_Signs2","DZ_Server_Data","DZ_Spawn_Proxies","DZ_Sounds_Arma2","DZ_Sounds_Mod","DZ_Sounds_Effects","DZ_Structures","DZ_Structures_A_BuildingWIP","DZ_Structures_A_CraneCon","DZ_Structures_A_MunicipalOffice","DZ_Structures_A_TVTower","DZ_Structures_Barn_W","DZ_Structures_Castle","DZ_Structures_Furniture","DZ_Structures_Houses","DZ_Structures_House_A_FuelStation","DZ_Structures_House_A_Hospital","DZ_Structures_House_A_Office01","DZ_Structures_House_A_Office02","DZ_Structures_House_A_StationHouse","DZ_Structures_House_Church_02","DZ_Structures_House_Church_03","DZ_Structures_House_Church_05R","DZ_Structures_House_HouseBT","DZ_Structures_House_HouseV","DZ_Structures_House_HouseV2","DZ_Structures_House_Street_Booth","DZ_Structures_Ind","DZ_Structures_Ind_Pipeline_IndPipe1","DZ_Structures_Ind_Quarry","DZ_Structures_Ind_SawMill","DZ_Structures_Mil","DZ_Structures_Misc","DZ_Structures_Misc_Powerlines","DZ_Structures_NAV","DZ_Structures_Nav_Boathouse","DZ_Structures_Nav_Pier","DZ_Structures_Pond","DZ_Structures_Rail","DZ_Structures_Rail_Rail_Station_big","DZ_Structures_Rail_Railway","DZ_Structures_Ruins","DZ_Structures_Shed_Ind","DZ_Structures_Shed_Shed_Small","DZ_Structures_Tenement","DZ_Structures_Wall","DZ_Structures_Ship_Wreck","DZ_UI","DZ_UIFonts","DZ_UIFonts_CoreConfigOverwrite","DZ_Vehicles_Tracked","DZ_Vehicles_Wheeled","DZ_Vehicles_Wheeled_Datsun_Armed","DZ_Vehicles_Wheeled_Hilux_Armed","DZ_Vehicles_Wrecks","DZ_Weapons_Magazines","DZ_Weapons_Lights","DZ_Weapons_Muzzles","DZ_Weapons_Optics","DZ_Weapons_Supports","DZ_Weapons_Animations","DZ_Weapons_Ammunition","DZ_Weapons_Firearms","DZ_Weapons_Firearms_Winchester70","DZ_Weapons_Firearms_SVD","DZ_Weapons_Melee","DZ_Weapons_Melee_Blunt","DZ_Weapons_Melee_Blade","DZ_Weapons_Melee_Powered","DZ_Weapons_Misc","DZ_Weapons_Explosives","DZ_Weapons_Firearms_aug","DZ_Weapons_Firearms_M4","DZ_Weapons_Firearms_MosinNagant","DZ_Weapons_Firearms_SKS","DZ_Weapons_Firearms_Ruger1022","DZ_Weapons_Pistols_1911","DZ_Weapons_Pistols_Cz75","DZ_Weapons_Pistols_Fnx45","DZ_Weapons_Pistols_Magnum","DZ_Weapons_Pistols_Mkii","DZ_Weapons_Projectiles","DZ_Weapons_Shotguns","DZ_Worlds_Test_Terrain"};
		};
	};
};
class CfgMods
{
	class VPPAdminTools
	{
		dir="VPPAdminTools";
		hideName=0;
		hidePicture=0;
		name="#STR_VPPAT_NAME";
		credits="DaOne & GravityWolf";
		author="DaOne & GravityWolf";
		authorID="420420";
		version="Version 0.7";
		extra=0;
		type="mod";
		inputs="VPPAdminTools/data/modded_Inputs.xml";
		picture="VPPAdminTools/data/vpp_logo_m.paa";
		logoSmall="VPPAdminTools/data/vpp_logo_ss.paa";
		logo="VPPAdminTools/data/vpp_logo_s.paa";
		logoOver="VPPAdminTools/data/vpp_logo_s.paa";
		tooltip="Dean Hall did nothing wrong. Make DayZ Greater than great.";
		overview="A DayZ Workshop Addon which aims to add administration tools by implementing a collection of features which help communities manager their players while keeping the installation, and configuration simple for new, and advanced users.";
		action="https://discord.gg/bkf3u7M";
		dependencies[]=
		{
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class widgetStyles
			{
				files[]=
				{
					"VPPAdminTools/GUI/Styles/testStyle.styles"
				};
			};
			class imageSets
			{
				files[]=
				{
					"VPPAdminTools/GUI/Textures/dayz_gui_vpp.imageset"
				};
			};
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"VPPAdminTools/Definitions",
					#ifndef VPPNOTIFICATIONS
					"VPPAdminTools/VPPNotifications/3_Game",
					#endif
					"VPPAdminTools/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"VPPAdminTools/Definitions",
					#ifndef VPPNOTIFICATIONS
					"VPPAdminTools/VPPNotifications/4_World",
					#endif
					"VPPAdminTools/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"VPPAdminTools/Definitions",
					#ifndef VPPNOTIFICATIONS
					"VPPAdminTools/VPPNotifications/5_Mission",
					#endif
					"VPPAdminTools/5_Mission"
				};
			};
		};
	};
};
#include "vbldr_DZStatics.cpp" //Map Objects for builder mode

#ifndef VPPNOTIFICATIONS //Only defines thigns below if VPPNotifications standalone version of the mod is not loaded
class CfgSoundShaders
{
	class UISEffect_0_SoundShaders
	{
		samples[]=
		{
			{
				"VPPAdminTools\sounds\beep",
				1
			}
		};
		range=25;
		volume=1.5;
	};
	class UISEffect_1_SoundShaders
	{
		samples[]=
		{
			{
				"VPPAdminTools\sounds\click",
				1
			}
		};
		range=25;
		volume=1.5;
	};
	class UISEffect_2_SoundShaders
	{
		samples[]=
		{
			{
				"VPPAdminTools\sounds\hint",
				1
			}
		};
		range=25;
		volume=1.5;
	};
};
class CfgSoundSets
{
	class UISEffect_0_SoundSets
	{
		soundShaders[]=
		{
			"UISEffect_0_SoundShaders"
		};
		volumeFactor=1;
		frequencyFactor=1;
		spatial=0;
	};
	class UISEffect_1_SoundSets
	{
		soundShaders[]=
		{
			"UISEffect_1_SoundShaders"
		};
		volumeFactor=1;
		frequencyFactor=1;
		spatial=0;
	};
	class UISEffect_2_SoundSets
	{
		soundShaders[]=
		{
			"UISEffect_2_SoundShaders"
		};
		volumeFactor=1;
		frequencyFactor=1;
		spatial=0;
	};
};
#endif
class RscMapControl
{
	scaleMin=4.9999999e-005;
	scaleMax=0.85000002;
	scaleDefault=0.85000002;
	ptsPerSquareSea=8;
	ptsPerSquareTxt=10;
	ptsPerSquareCLn=10;
	ptsPerSquareExp=10;
	ptsPerSquareCost=10;
	ptsPerSquareFor=99;
	ptsPerSquareForEdge=99;
	ptsPerSquareRoad=4;
	ptsPerSquareObj=15;
	maxSatelliteAlpha=1;
	alphaFadeStartScale=1;
	alphaFadeEndScale=1;
	userMapPath="dz\gear\navigation\data\usermap";
	maxUserMapAlpha=0.2;
	alphaUserMapFadeStartScale=0.5;
	alphaUserMapFadeEndScale=0.80000001;
	showCountourInterval=1;
	colorLevels[]={0.64999998,0.60000002,0.44999999,0.30000001};
	colorSea[]={0.2,0.5,0.69999999,1};
	colorForest[]={0.36000001,0.77999997,0.079999998,0};
	colorRocks[]={0.5,0.5,0.5,0.2};
	colorCountlines[]={0.85000002,0.80000001,0.64999998,0.1};
	colorMainCountlines[]={0.44999999,0.40000001,0.25,0};
	colorCountlinesWater[]={0.25,0.40000001,0.5,0.30000001};
	colorMainCountlinesWater[]={0.25,0.40000001,0.5,0.89999998};
	colorPowerLines[]={0.1,0.1,0.1,1};
	colorRailWay[]={0.80000001,0.2,0,1};
	colorForestBorder[]={0.40000001,0.80000001,0,0};
	colorRocksBorder[]={0.5,0.5,0.5,0};
	colorOutside[]={1,1,1,1};
	colorTracks[]={0.77999997,0.66000003,0.34,1};
	colorRoads[]={0.69,0.43000001,0.23,1};
	colorMainRoads[]={0.52999997,0.34999999,0,1};
	colorTracksFill[]={0.95999998,0.91000003,0.60000002,1};
	colorRoadsFill[]={0.92000002,0.73000002,0.41,1};
	colorMainRoadsFill[]={0.83999997,0.61000001,0.20999999,1};
	colorGrid[]={0.89999998,0.89999998,0.89999998,0.80000001};
	colorGridMap[]={0.89999998,0.89999998,0.89999998,0.2};
	fontNames="gui/fonts/sdf_MetronBook24";
	sizeExNames=0.029999999;
	colorNames[]={1,1,1,1};
	fontGrid="gui/fonts/sdf_MetronBook24";
	sizeExGrid=0.02;
	fontLevel="gui/fonts/sdf_MetronBook24";
	sizeExLevel=0.0099999998;
	colorMountPoint[]={0.44999999,0.40000001,0.25,0};
	mapPointDensity=0.12;
	text="";
	fontLabel="gui/fonts/sdf_MetronBook24";
	fontInfo="gui/fonts/sdf_MetronBook24";
	class Legend
	{
		x=0.050000001;
		y=0.85000002;
		w=0.40000001;
		h=0.1;
		font="gui/fonts/sdf_MetronBook24";
		sizeEx=0.02;
		colorBackground[]={1,1,1,0};
		color[]={0,0,0,0};
	};
	class Bush
	{
		icon="\dz\gear\navigation\data\map_bush_ca.paa";
		color[]={0.40000001,0.80000001,0,0};
		size=14;
		importance="0.2 * 14 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class SmallTree
	{
		icon="\dz\gear\navigation\data\map_smalltree_ca.paa";
		color[]={0.40000001,0.80000001,0,0};
		size=12;
		importance="0.6 * 12 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Tree
	{
		icon="\dz\gear\navigation\data\map_tree_ca.paa";
		color[]={0.40000001,0.80000001,0,0};
		size=12;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Rock
	{
		icon="\dz\gear\navigation\data\map_rock_ca.paa";
		color[]={0.1,0.1,0.1,0.80000001};
		size=12;
		importance="0.5 * 12 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fuelstation
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\fuelstation.paa";
		size=16;
		color[]={1,1,1,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.75;
		coefMax=2;
	};
	class Lighthouse
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\lighthouse.paa";
		size=30;
		color[]={1,1,1,1};
		importance="0.9 * 16 * 0.05";
		coefMin=1;
		coefMax=4;
	};
	class Stack
	{
		icon="\dz\gear\navigation\data\map_stack_ca.paa";
		size=16;
		color[]={1,1,1,0};
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Transmitter
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\radiotower.paa";
		color[]={1,1,1,1};
		size=30;
		importance="0.9 * 16 * 0.05";
		coefMin=0.69999999;
		coefMax=4;
	};
	class Watertower
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\watertower.paa";
		color[]={1,1,1,1};
		size=30;
		importance="0.9 * 16 * 0.05";
		coefMin=0.69999999;
		coefMax=4;
	};
	class Shipwreck
	{
		icon="\dz\gear\navigation\data\map_shipwreck_ca.paa";
		color[]={1,1,1,1};
		size=16;
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Monument
	{
		icon="\dz\gear\navigation\data\map_monument_ca.paa";
		color[]={1,1,1,0};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class BusStop
	{
		icon="\dz\gear\navigation\data\map_busstop_ca.paa";
		color[]={1,1,1,0};
		size=16;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fountain
	{
		icon="\dz\gear\navigation\data\map_fountain_ca.paa";
		color[]={1,1,1,0};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Waterpump
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\waterpump.paa";
		color[]={1,1,1,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class Hospital
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\hospital.paa";
		color[]={1,1,1,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class Store
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\shopping.paa";
		color[]={1,1,1,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class Police
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\police.paa";
		color[]={1,1,1,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class FireDep
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\firestation.paa";
		color[]={1,1,1,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class TouristSign
	{
		icon="\dz\gear\navigation\data\map_tsign_ca.paa";
		color[]={1,1,1,0};
		size=15;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Tourism
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\bar.paa";
		color[]={1,1,1,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class Cross
	{
		icon="\dz\gear\navigation\data\map_cross_ca.paa";
		size=20;
		color[]={1,1,0,0};
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Chapel
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\church.paa";
		color[]={1,1,1,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=2;
	};
	class Church
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\church.paa";
		color[]={1,1,1,1};
		size=35;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=2;
	};
	class ViewTower
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\watchtower.paa";
		color[]={1,1,1,1};
		size=22;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class TouristShelter
	{
		icon="\dz\gear\navigation\data\map_tshelter_ca.paa";
		color[]={1,1,1,1};
		size=18;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Quay
	{
		icon="\dz\gear\navigation\data\map_quay_ca.paa";
		size=16;
		color[]={1,1,1,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Bunker
	{
		icon="\dz\gear\navigation\data\map_bunker_ca.paa";
		color[]={0.60000002,0.60000002,0.60000002,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fortress
	{
		icon="\dz\gear\navigation\data\map_fortress_ca.paa";
		size=20;
		color[]={0.60000002,0.60000002,0.60000002,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Rocks
	{
		icon="\dz\gear\navigation\data\map_rock_ca.paa";
		color[]={0.60000002,0.60000002,0.60000002,1};
		size=12;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Ruin
	{
		icon="\dz\gear\navigation\data\map_ruin_ca.paa";
		size=16;
		color[]={0.60000002,0.60000002,0.60000002,1};
		importance="1.2 * 16 * 0.05";
		coefMin=1;
		coefMax=4;
	};
};
class CfgLocationTypes
{
	class Name
	{
		name="keypoint";
		drawStyle="name";
		texture="#(argb,1,1,1)color(1,1,1,1)";
		color[]={1,1,1,1};
		size=0;
		font="gui/fonts/sdf_MetronBook24";
		textSize=0.029999999;
		shadow=0.1;
		importance=1;
	};
	class NameIcon
	{
		name="keypoint";
		drawStyle="icon";
		texture="#(argb,1,1,1)color(1,1,1,1)";
		color[]={1,1,1,1};
		size=0;
		font="gui/fonts/sdf_MetronBook24";
		textSize=0.029999999;
		shadow=0;
		importance=1;
	};
	class Capital: Name
	{
		color[]={1,1,1,1};
		textSize=0.059999999;
		importance=7;
	};
	class BigMilitary: Name
	{
		color[]={1,0.1,0.1,1};
		textSize=0.050000001;
		importance=6;
	};
	class City: Name
	{
		textSize=0.050000001;
		importance=6;
	};
	class MediumMilitary: Name
	{
		color[]={1,0.1,0.1,1};
		textSize=0.039999999;
		importance=5;
	};
	class SmallMilitary: Name
	{
		color[]={1,0.1,0.1,1};
		textSize=0.035;
		importance=5;
	};
	class Village: Name
	{
		textSize=0.039999999;
		importance=3;
	};
	class Local: Name
	{
		color[]={0.44,0.38,0.30000001,1};
		textSize=0.029999999;
	};
	class Marine: Name
	{
		color[]={0.050000001,0.40000001,0.80000001,0.80000001};
		textSize=0.029999999;
	};
	class FeedShack: NameIcon
	{
		texture="\VanillaPPMap\GUI\Textures\CustomMapIcons\feedshack.paa";
		importance=1;
		size=10;
	};
	class Deerstand: NameIcon
	{
		texture="\VanillaPPMap\GUI\Textures\CustomMapIcons\deerstand.paa";
		importance=1;
		size=10;
	};
	class Ruin: NameIcon
	{
		texture="\DZ\gear\navigation\data\map_ruin_ca.paa";
		textSize=0.029999999;
		importance=2;
		size=7;
	};
	class Camp: NameIcon
	{
		color[]={0.44,0.38,0.30000001,1};
		texture="\DZ\gear\navigation\data\map_camp_ca.paa";
		textSize=0.02;
		size=5;
	};
	class Hill: NameIcon
	{
		color[]={0.69999999,0.69999999,0.69999999,1};
		texture="\DZ\gear\navigation\data\map_hill_ca.paa";
		textSize=0.02;
		size=3;
	};
	class ViewPoint: NameIcon
	{
		color[]={0.77999997,0,0.050000001,0};
		texture="\DZ\gear\navigation\data\map_viewpoint_ca.paa";
		size=9;
	};
	class RockArea: NameIcon
	{
		color[]={0,0,0,1};
		texture="\DZ\gear\navigation\data\map_rock_ca.paa";
		size=9;
	};
	class RailroadStation: NameIcon
	{
		color[]={0,0,0,0};
		texture="\DZ\gear\navigation\data\map_station_ca.paa";
		size=9;
	};
	class IndustrialSite: NameIcon
	{
		color[]={0,0,0,1};
		texture="\DZ\gear\navigation\data\map_factory_ca.paa";
		size=9;
	};
	class LocalOffice: NameIcon
	{
		color[]={0.22,0.20999999,0.50999999,0};
		texture="\DZ\gear\navigation\data\map_govoffice_ca.paa";
		size=10;
	};
	class BorderCrossing: NameIcon
	{
		color[]={0.77999997,0,0.050000001,1};
		texture="\DZ\gear\navigation\data\map_border_cross_ca.paa";
		size=9;
	};
	class VegetationBroadleaf: NameIcon
	{
		color[]={0.25,0.40000001,0.2,1};
		texture="\DZ\gear\navigation\data\map_broadleaf_ca.paa";
		size=9;
	};
	class VegetationFir: NameIcon
	{
		color[]={0.25,0.40000001,0.2,1};
		texture="\DZ\gear\navigation\data\map_fir_ca.paa";
		size=9;
	};
	class VegetationPalm: NameIcon
	{
		color[]={0.25,0.40000001,0.2,1};
		texture="\DZ\gear\navigation\data\map_palm_ca.paa";
		size=9;
	};
	class VegetationVineyard: NameIcon
	{
		color[]={0.25,0.40000001,0.2,1};
		texture="\DZ\gear\navigation\data\map_vineyard_ca.paa";
		size=9;
	};
};
class VPPMapSettings: RscMapControl
{
	scaleMin=0.85000002;
	scaleMax=0.85000002;
	scaleDefault=0.85000002;
};
class CfgWorlds
{
	class CAWorld;
	class ChernarusPlus: CAWorld
	{
		class Names
		{
			class Settlement_Chernogorsk
			{
				name="Chernogorsk";
				position[]={6778.6191,2321.7739};
				type="Capital";
			};
			class Settlement_Novodmitrovsk
			{
				name="Novodmitrovsk";
				position[]={11339.77,14391.05};
				type="Capital";
			};
			class Settlement_Novoselki
			{
				name="Novoselki";
				position[]={6115.9102,3256.3601};
				type="City";
			};
			class Settlement_Dubovo
			{
				name="Dubovo";
				position[]={6754.5098,3595.2571};
				type="City";
			};
			class Settlement_Vysotovo
			{
				name="Vysotovo";
				position[]={5701.2402,2556.1101};
				type="City";
			};
			class Settlement_Zelenogorsk
			{
				name="Zelenogorsk";
				position[]={2796,5166};
				type="City";
			};
			class Settlement_Berezino
			{
				name="Berezino";
				position[]={12372.383,9760.9814};
				type="City";
			};
			class Settlement_Elektrozavodsk
			{
				name="Elektrozavodsk";
				position[]={10286.68,1992.77};
				type="City";
			};
			class Settlement_NovayaPetrovka
			{
				name="Novaya Petrovka";
				position[]={3477.1689,12931.243};
				type="City";
			};
			class Settlement_Gorka
			{
				name="Gorka";
				position[]={9524,8945};
				type="City";
			};
			class Settlement_Solnechny
			{
				name="Solnechny";
				position[]={13487.724,6225.8838};
				type="City";
			};
			class Settlement_StarySobor
			{
				name="Stary Sobor";
				position[]={6114,7849};
				type="City";
			};
			class Settlement_Vybor
			{
				name="Vybor";
				position[]={3909.8311,9019.6143};
				type="City";
			};
			class Settlement_Severograd
			{
				name="Severograd";
				position[]={8168.4839,12573.602};
				type="City";
			};
			class Settlement_Bor
			{
				name="Bor";
				position[]={3316.406,4070.9661};
				type="Village";
			};
			class Settlement_Svetloyarsk
			{
				name="Svetloyarsk";
				position[]={13989.74,13263.78};
				type="City";
			};
			class Settlement_Krasnostav
			{
				name="Krasnostav";
				position[]={11203.167,12340.808};
				type="City";
			};
			class Settlement_ChernayaPolyana
			{
				name="Chernaya Polyana";
				position[]={12158.54,13770.22};
				type="City";
			};
			class Settlement_Polyana
			{
				name="Polyana";
				position[]={10794,8160};
				type="City";
			};
			class Settlement_Tulga
			{
				name="Tulga";
				position[]={12768,4384};
				type="Village";
			};
			class Settlement_Msta
			{
				name="Msta";
				position[]={11333.79,5421.3999};
				type="Village";
			};
			class Settlement_Staroye
			{
				name="Staroye";
				position[]={10212.52,5385.0498};
				type="Village";
			};
			class Settlement_Shakhovka
			{
				name="Shakhovka";
				position[]={9728.6396,6567.2998};
				type="Village";
			};
			class Settlement_Dolina
			{
				name="Dolina";
				position[]={11285.17,6633.5801};
				type="Village";
			};
			class Settlement_Orlovets
			{
				name="Orlovets";
				position[]={12250,7253};
				type="Village";
			};
			class Settlement_NovySobor
			{
				name="Novy Sobor";
				position[]={7123.6299,7774.6299};
				type="Village";
			};
			class Settlement_Kabanino
			{
				name="Kabanino";
				position[]={5300.71,8645.9004};
				type="Village";
			};
			class Settlement_Mogilevka
			{
				name="Mogilevka";
				position[]={7650.4102,5096.46};
				type="Village";
			};
			class Settlement_Nadezhdino
			{
				name="Nadezhdino";
				position[]={5894,4780};
				type="Village";
			};
			class Settlement_Guglovo
			{
				name="Guglovo";
				position[]={8445,6579};
				type="Village";
			};
			class Settlement_Kamyshovo
			{
				name="Kamyshovo";
				position[]={12170,3458};
				type="Village";
			};
			class Settlement_Pusta
			{
				name="Pusta";
				position[]={9197,3873};
				type="Village";
			};
			class Settlement_Dubrovka
			{
				name="Dubrovka";
				position[]={10324.299,9888.2373};
				type="Village";
			};
			class Settlement_VyshnayaDubrovka
			{
				name="Vyshnaya Dubrovka";
				position[]={9926.8232,10468.919};
				type="Village";
			};
			class Settlement_Khelm
			{
				name="Khelm";
				position[]={12329.27,10771.01};
				type="Village";
			};
			class Settlement_Olsha
			{
				name="Olsha";
				position[]={13402,12898};
				type="Village";
			};
			class Settlement_Gvozdno
			{
				name="Gvozdno";
				position[]={8644.751,11934.115};
				type="Village";
			};
			class Settlement_Grishino
			{
				name="Grishino";
				position[]={5980,10257};
				type="Village";
			};
			class Settlement_Rogovo
			{
				name="Rogovo";
				position[]={4802,6748};
				type="Village";
			};
			class Settlement_Pogorevka
			{
				name="Pogorevka";
				position[]={4445,6322};
				type="Village";
			};
			class Settlement_Pustoshka
			{
				name="Pustoshka";
				position[]={3137.9441,7890.9248};
				type="Village";
			};
			class Settlement_Kozlovka
			{
				name="Kozlovka";
				position[]={4402,4736};
				type="Village";
			};
			class Settlement_Karmanovka
			{
				name="Karmanovka";
				position[]={12700.628,14691.312};
				type="Village";
			};
			class Settlement_Balota
			{
				name="Balota";
				position[]={4527.5801,2465.3201};
				type="Village";
			};
			class Settlement_Komarovo
			{
				name="Komarovo";
				position[]={3702,2440};
				type="Village";
			};
			class Settlement_Kamenka
			{
				name="Kamenka";
				position[]={1981,2207};
				type="Village";
			};
			class Settlement_Myshkino
			{
				name="Myshkino";
				position[]={2048,7258};
				type="Village";
			};
			class Settlement_Pavlovo
			{
				name="Pavlovo";
				position[]={1693.98,3858.6499};
				type="Village";
			};
			class Settlement_Lopatino
			{
				name="Lopatino";
				position[]={2801,10029};
				type="Village";
			};
			class Settlement_Vyshnoye
			{
				name="Vyshnoye";
				position[]={6629,6051};
				type="Village";
			};
			class Settlement_Prigorodki
			{
				name="Prigorodki";
				position[]={7760,3336};
				type="Village";
			};
			class Settlement_Drozhino
			{
				name="Drozhino";
				position[]={3374,4861};
				type="Village";
			};
			class Settlement_Sosnovka
			{
				name="Sosnovka";
				position[]={2556,6307};
				type="Village";
			};
			class Settlement_Nizhneye
			{
				name="Nizhneye";
				position[]={12824.88,8097.7002};
				type="Village";
			};
			class Settlement_Pulkovo
			{
				name="Pulkovo";
				position[]={5015,5607};
				type="Village";
			};
			class Settlement_Berezhki
			{
				name="Berezhki";
				position[]={15020,13905};
				type="Village";
			};
			class Settlement_Turovo
			{
				name="Turovo";
				position[]={13630,14146};
				type="Village";
			};
			class Settlement_BelayaPolyana
			{
				name="Belaya Polyana";
				position[]={14166,14968};
				type="Village";
			};
			class Settlement_Dobroye
			{
				name="Dobroye";
				position[]={12996,15066};
				type="Village";
			};
			class Settlement_Nagornoye
			{
				name="Nagornoye";
				position[]={9286,14677};
				type="Village";
			};
			class Settlement_Svergino
			{
				name="Svergino";
				position[]={9493.5195,13875.27};
				type="Village";
			};
			class Settlement_Ratnoye
			{
				name="Ratnoye";
				position[]={6195.8779,12761.802};
				type="Village";
			};
			class Settlement_Kamensk
			{
				name="Kamensk";
				position[]={6748.7412,14430.046};
				type="Village";
			};
			class Settlement_Krasnoye
			{
				name="Krasnoye";
				position[]={6411.374,15029.961};
				type="Village";
			};
			class Settlement_StaryYar
			{
				name="StaryYar";
				position[]={4987.8599,14992.976};
				type="Village";
			};
			class Settlement_Polesovo
			{
				name="Polesovo";
				position[]={5955.7368,13558.046};
				type="Village";
			};
			class Settlement_Tisy
			{
				name="Tisy";
				position[]={3441.6499,14799.55};
				type="Village";
			};
			class Settlement_Topolniki
			{
				name="Topolniki";
				position[]={2913.7451,12366.357};
				type="Village";
			};
			class Settlement_Zaprudnoye
			{
				name="Zaprudnoye";
				position[]={5214,12792};
				type="Village";
			};
			class Settlement_Sinystok
			{
				name="Sinystok";
				position[]={1538.278,11904.578};
				type="Village";
			};
			class Settlement_Vavilovo
			{
				name="Vavilovo";
				position[]={2263,11103};
				type="Village";
			};
			class Settlement_Kumyrna
			{
				name="Kumyrna";
				position[]={8300,6054};
				type="Village";
			};
			class Settlement_Kalinovka
			{
				name="Kalinovka";
				position[]={7448,13406};
				type="Village";
			};
			class Settlement_Bogatyrka
			{
				name="Bogatyrka";
				position[]={1453.62,8870.9199};
				type="Village";
			};
			class Settlement_SvyatoyRomanSkiResort
			{
				name="Ski Resort";
				position[]={638,11667};
				type="Local";
			};
			class Settlement_SKVSChBiathlonArena
			{
				name="Biathlon Arena";
				position[]={357,11142};
				type="Local";
			};
			class Settlement_Zvir
			{
				name="Zvir";
				position[]={577.33002,5317.4199};
				type="Village";
			};
			class Settlement_Zabolotye
			{
				name="Zabolotye";
				position[]={1256.859,9993.2598};
				type="Village";
			};
			class Settlement_Galkino
			{
				name="Galkino";
				position[]={1102.6801,8798.2695};
				type="Local";
			};
			class Camp_Arsenovo
			{
				name="Arsenovo";
				position[]={8509.6611,13918.628};
				type="Camp";
			};
			class Camp_Stroytel
			{
				name="Stroytel";
				position[]={7017.6299,4365.21};
				type="Camp";
			};
			class Camp_Romashka
			{
				name="Romashka";
				position[]={8177.6021,10980.289};
				type="Camp";
			};
			class Camp_Kometa
			{
				name="Kometa";
				position[]={10271.697,3558.228};
				type="Camp";
			};
			class Camp_Druzhba
			{
				name="Druzhba";
				position[]={11437,10695};
				type="Camp";
			};
			class Camp_Nadezhda
			{
				name="Nadezhda";
				position[]={7279.9678,7014.3501};
				type="Camp";
			};
			class Camp_YouthPioneer
			{
				name="Youth Pioneer";
				position[]={11155.825,7074.1431};
				type="Camp";
			};
			class Camp_ProudChernarus
			{
				name="Proud Chernarus";
				position[]={3199.6169,6174.9321};
				type="Camp";
			};
			class Camp_Shkolnik
			{
				name="Shkolnik";
				position[]={14856.503,14571.028};
				type="Camp";
			};
			class Camp_Pobeda
			{
				name="Pobeda";
				position[]={3703.3821,14876.384};
				type="Camp";
			};
			class Camp_Metalurg
			{
				name="Metalurg";
				position[]={1035.406,6677.3828};
				type="Camp";
			};
			class Hill_Zelenayagora
			{
				name="Green Mountain";
				position[]={3767.1699,6010.54};
				type="Village";
			};
			class Local_Dichina
			{
				name="Dichina";
				position[]={4618,7821};
				type="Local";
			};
			class Local_Novylug
			{
				name="Novylug";
				position[]={9251,11360};
				type="Local";
			};
			class Local_Staryeluga
			{
				name="Staryeluga";
				position[]={6919,8995};
				type="Local";
			};
			class Hill_Vysota
			{
				name="Vysota";
				position[]={6591.6299,3400};
				type="Hill";
			};
			class Hill_Kopyto
			{
				name="Kopyto";
				position[]={7871.479,3869.1021};
				type="Hill";
			};
			class Local_Grubyeskaly
			{
				name="Grubyeskaly";
				position[]={13115.06,11900};
				type="Local";
			};
			class Local_Chernyles
			{
				name="Chernyles";
				position[]={9031,7803};
				type="Local";
			};
			class Hill_Altar
			{
				name="Altar";
				position[]={8143.3999,9159.6797};
				type="Hill";
			};
			class Local_RadioZenit
			{
				name="Radio Zenit";
				position[]={8080.0098,9341.6797};
				type="Local";
			};
			class Hill_PikKozlova
			{
				name="Pik Kozlova";
				position[]={8850.1699,2880.53};
				type="Hill";
			};
			class Local_Pustoykhrebet
			{
				name="Pustoykhrebet";
				position[]={10890,5665};
				type="Local";
			};
			class Hill_Bashnya
			{
				name="Bashnya";
				position[]={4178.27,11771.22};
				type="Hill";
			};
			class Hill_Veresnik
			{
				name="Veresnik";
				position[]={4440.1699,8070.54};
				type="Hill";
			};
			class Hill_Kurgan
			{
				name="Kurgan";
				position[]={3368.52,5296.8701};
				type="Hill";
			};
			class Hill_Kustryk
			{
				name="Kustryk";
				position[]={4912.8799,5063.4502};
				type="Hill";
			};
			class Hill_Vetryanayagora
			{
				name="Vetryanayagora";
				position[]={3892.74,4200.5898};
				type="Hill";
			};
			class Hill_Kalmyk
			{
				name="Kalmyk";
				position[]={6903.7798,4919.6602};
				type="Hill";
			};
			class Hill_PopIvan
			{
				name="PopIvan";
				position[]={6420.2598,6570.6602};
				type="Hill";
			};
			class Hill_Erbenka
			{
				name="Erbenka";
				position[]={2842.916,4014.811};
				type="Hill";
			};
			class Local_Lesnoykhrebet
			{
				name="Lesnoykhrebet";
				position[]={8122.77,7815.54};
				type="Local";
			};
			class Hill_Vysokiykamen
			{
				name="Vysokiykamen";
				position[]={8940.1904,4380.52};
				type="Hill";
			};
			class Hill_Dobry
			{
				name="Dobry";
				position[]={10552.9,3061.03};
				type="Hill";
			};
			class Hill_Baranchik
			{
				name="Baranchik";
				position[]={10153.147,6208.521};
				type="Hill";
			};
			class Hill_Malinovka
			{
				name="Malinovka";
				position[]={10897.7,7575.5898};
				type="Hill";
			};
			class Hill_Dubina
			{
				name="Dubina";
				position[]={11107.9,8474.8301};
				type="Hill";
			};
			class Hill_Klen
			{
				name="Klen";
				position[]={11473.511,11315.393};
				type="Hill";
			};
			class Hill_Chernayagora
			{
				name="Black Mountain";
				position[]={10280.816,12053.928};
				type="Hill";
			};
			class Ruin_Zolotar
			{
				name="Zolotar";
				position[]={10179,11998};
				type="Ruin";
			};
			class Hill_Ostry
			{
				name="Ostry";
				position[]={10792.747,12829.504};
				type="Hill";
			};
			class Hill_Olsha
			{
				name="Olsha";
				position[]={12975.7,12775.2};
				type="Hill";
			};
			class Marine_Tikhiyzaliv
			{
				name="Tikhiyzaliv";
				position[]={1221.49,2111.8899};
				type="Marine";
			};
			class Marine_Mutnyizaliv
			{
				name="Mutnyizaliv";
				position[]={5735.479,1918.92};
				type="Marine";
			};
			class Marine_Chernyzaliv
			{
				name="Chernyzaliv";
				position[]={7599.0688,2486.5859};
				type="Marine";
			};
			class Marine_Zelenyzaliv
			{
				name="Zelenyzaliv";
				position[]={11227.019,2989.8379};
				type="Marine";
			};
			class Marine_Skalistyproliv
			{
				name="Skalistyproliv";
				position[]={13385.92,3613.9399};
				type="Marine";
			};
			class Marine_Nizhniyzaliv
			{
				name="Nizhniyzaliv";
				position[]={12989.3,8515.7598};
				type="Marine";
			};
			class Marine_ZalivGuba
			{
				name="Zaliv Guba";
				position[]={14328.374,13136.732};
				type="Marine";
			};
			class Marine_Rify
			{
				name="Rify";
				position[]={13931.73,11288.61};
				type="Local";
			};
			class Marine_Ivovoyeozero
			{
				name="Ivovoyeozero";
				position[]={13248,11572};
				type="Marine";
			};
			class Marine_Chernoyeozero
			{
				name="Chernoyeozero";
				position[]={13381,12002};
				type="Marine";
			};
			class Marine_PlotinaTopolka
			{
				name="Topolka Dam";
				position[]={10231.33,3691.1499};
				type="Marine";
			};
			class Marine_PlotinaPobeda
			{
				name="Plotina Pobeda";
				position[]={9928.2939,13874.643};
				type="Marine";
			};
			class Marine_PlotinaTishina
			{
				name="Plotina Tishina";
				position[]={1150.8199,6432.4541};
				type="Marine";
			};
			class Marine_Ozerko
			{
				name="Ozerko";
				position[]={6777.6299,4492.4199};
				type="Marine";
			};
			class Marine_Prud
			{
				name="Prud";
				position[]={6610.98,9308.5703};
				type="Marine";
			};
			class Ruin_Chortovzamok
			{
				name="Devil's Castle";
				position[]={6883.2388,11501.288};
				type="Ruin";
			};
			class Ruin_Zub
			{
				name="Zub";
				position[]={6574.2798,5573.8501};
				type="Ruin";
			};
			class Ruin_Rog
			{
				name="Rog";
				position[]={11267.206,4293.0361};
				type="Ruin";
			};
			class Local_Grozovypereval
			{
				name="Pass Grozovy";
				position[]={3318.1201,15250.55};
				type="Local";
			};
			class Local_Sosnovypereval
			{
				name="Pass Sosnovy";
				position[]={2687.3999,6590.2798};
				type="Local";
			};
			class Local_PerevalOreshka
			{
				name="Pass Oreshka";
				position[]={9822.1914,6088.793};
				type="Local";
			};
			class Local_Turovskiypereval
			{
				name="Pass Turovskiy";
				position[]={14674.241,14089.306};
				type="Local";
			};
			class Local_Tridoliny
			{
				name="Tridoliny";
				position[]={12764.47,5412.21};
				type="Local";
			};
			class Local_Grozovyedoly
			{
				name="Grozovyedoly";
				position[]={10403.982,14793.168};
				type="Local";
			};
			class Ruin_Klyuch
			{
				name="Klyuch";
				position[]={9282.5703,13476.67};
				type="Ruin";
			};
			class Hill_Lysayagora
			{
				name="Lysayagora";
				position[]={6722.459,14050.42};
				type="Hill";
			};
			class Marine_Glubokoyeozero
			{
				name="Glubokoyeozero";
				position[]={1692.5551,14821.565};
				type="Marine";
			};
			class Local_Skalka
			{
				name="Skalka";
				position[]={5758,14486};
				type="Local";
			};
			class Local_Vidy
			{
				name="Vidy";
				position[]={785.05402,14398.815};
				type="Local";
			};
			class Hill_Tumannyverkh
			{
				name="Tumannyverkh";
				position[]={331.224,12982.99};
				type="Hill";
			};
			class Local_Adamovka
			{
				name="Adamovka";
				position[]={5340.6602,11380.01};
				type="Local";
			};
			class Hill_Shishok
			{
				name="Shishok";
				position[]={3559.313,9422.2695};
				type="Hill";
			};
			class Settlement_Skalisty
			{
				name="Skalisty";
				position[]={13715.454,3118.78};
				type="Village";
			};
			class Ruin_Storozh
			{
				name="Storozh";
				position[]={2816.6011,1277.96};
				type="Ruin";
			};
			class Local_MysGolova
			{
				name="Mys Golova";
				position[]={8286.0527,2405.5979};
				type="Local";
			};
			class Local_Drakon
			{
				name="Drakon";
				position[]={11191,2443};
				type="Local";
			};
			class Local_Otmel
			{
				name="Otmel";
				position[]={11581.25,3213.24};
				type="Local";
			};
			class Local_MysKrutoy
			{
				name="Mys Krutoy";
				position[]={13578.19,3976.8201};
				type="Local";
			};
			class Hill_Tokarnya
			{
				name="Tokarnya";
				position[]={8890.4775,5672.5532};
				type="Hill";
			};
			class Hill_Ostrog
			{
				name="Ostrog";
				position[]={2655.7339,2264.6321};
				type="Hill";
			};
			class Local_Maryanka
			{
				name="Maryanka";
				position[]={2789,3386};
				type="Local";
			};
			class Local_Polonina
			{
				name="Polonina";
				position[]={1004.486,4242.8691};
				type="Local";
			};
			class Local_Kalinka
			{
				name="Kalinka";
				position[]={3346.5,11292.097};
				type="Local";
			};
			class Hill_Kikimora
			{
				name="Kikimora";
				position[]={1812,6200};
				type="Hill";
			};
			class Hill_BolshoyKotel
			{
				name="Bolshoy Kotel";
				position[]={714.64899,6520.4312};
				type="Hill";
			};
			class Hill_Simurg
			{
				name="Simurg";
				position[]={134,7569.9902};
				type="Hill";
			};
			class Hill_Volchiypik
			{
				name="Volchiypik";
				position[]={320.254,2719.269};
				type="Hill";
			};
			class Ruin_Krona
			{
				name="Krona";
				position[]={1493,9270.3398};
				type="Ruin";
			};
			class Local_TriKresta
			{
				name="Tri Kresta";
				position[]={301,9478};
				type="Local";
			};
			class Local_Rostoki
			{
				name="Rostoki";
				position[]={744.45001,8647.8604};
				type="Local";
			};
			class Marine_OrlyeOzero
			{
				name="Orlye Ozero";
				position[]={597,5990};
				type="Marine";
			};
			class Local_Makosh
			{
				name="Makosh";
				position[]={7849.7202,6480.1899};
				type="Local";
			};
			class Local_Klenovyipereval
			{
				name="Klenovyipereval";
				position[]={10862.702,11585.512};
				type="Local";
			};
			class Local_Zmeinykhrebet
			{
				name="Zmeinykhrebet";
				position[]={11446.17,13606.92};
				type="Local";
			};
			class Hill_Sokol
			{
				name="Sokol";
				position[]={12001,14813};
				type="Hill";
			};
			class Local_Krutyeskaly
			{
				name="Krutyeskaly";
				position[]={14880.63,13671.76};
				type="Local";
			};
			class Local_Bogat
			{
				name="Bogat";
				position[]={7058.0112,12023.571};
				type="Local";
			};
			class Local_Dubnik
			{
				name="Dubnik";
				position[]={3298.29,10330.89};
				type="Local";
			};
			class Hill_Viselitsa
			{
				name="Viselitsa";
				position[]={12722.157,7504.0498};
				type="Hill";
			};
			class Local_Dazhbog
			{
				name="Dazhbog";
				position[]={6581,8590};
				type="Hill";
			};
			class Marine_Verbnik
			{
				name="Verbnik";
				position[]={4414.8789,9103.002};
				type="Marine";
			};
			class Local_Medvezhilugi
			{
				name="Medvezhilugi";
				position[]={9641,13236};
				type="Local";
			};
			class Ruin_Voron
			{
				name="Voron";
				position[]={13517.93,3311.2451};
				type="Ruin";
			};
			class Ruin_Gnomovzamok
			{
				name="Gnomovzamok";
				position[]={7446.915,9097.1699};
				type="Ruin";
			};
			class Marine_Glaza
			{
				name="Glaza";
				position[]={7383,9320};
				type="Marine";
			};
			class Local_KarerKrasnayaZarya
			{
				name="Karer Krasnaya Zarya";
				position[]={8565.3486,13418.684};
				type="Local";
			};
			class Local_Matveyevo
			{
				name="Matveyevo";
				position[]={4281,7380};
				type="Local";
			};
			class Local_Kotka
			{
				name="Kotka";
				position[]={5873.7002,6883.2402};
				type="Local";
			};
			class Local_Chernyeskaly
			{
				name="Chernyeskaly";
				position[]={3932.572,14599.496};
				type="Local";
			};
			class Hill_SvyatoyRoman
			{
				name="Svyatoy Roman";
				position[]={84.832001,11901.246};
				type="Hill";
			};
			class Hill_Koman
			{
				name="Koman";
				position[]={1660.652,7415.6611};
				type="Hill";
			};
			class Hill_Mayak
			{
				name="Mayak";
				position[]={12323.813,4553.1069};
				type="Hill";
			};
			class Local_MB_Berezino
			{
				name="Berezino Military";
				position[]={11966,9130};
				type="SmallMilitary";
			};
			class Local_MB_NovyLug
			{
				name="Novy Lug Military";
				position[]={9579,11796};
				type="SmallMilitary";
			};
			class Local_MB_Elektrozavodsk
			{
				name="Elektrozavodsk Military";
				position[]={10481,2356};
				type="SmallMilitary";
			};
			class Local_MB_Turovo
			{
				name="Turovo Military";
				position[]={13626,13740};
				type="SmallMilitary";
			};
			class Local_MB_StarySobor
			{
				name="Stary Sobor Military";
				position[]={6351,7790};
				type="MediumMilitary";
			};
			class Local_MB_Sosnovy
			{
				name="Sosnovy Military";
				position[]={2720,6760};
				type="SmallMilitary";
			};
			class Local_MB_Ostry
			{
				name="Ostry Military";
				position[]={10958,13341};
				type="MediumMilitary";
			};
			class Local_MB_Severograd
			{
				name="Severograd Military";
				position[]={7074,12735};
				type="SmallMilitary";
			};
			class Local_MB_Arsenovo
			{
				name="Arsenovo Military";
				position[]={8560,14000};
				type="SmallMilitary";
			};
			class Local_MB_KamenskSmall
			{
				name="Kamensk Military";
				position[]={7075,14520};
				type="SmallMilitary";
			};
			class Local_MB_Grozovy
			{
				name="Grozovy Military";
				position[]={3660,14920};
				type="SmallMilitary";
			};
			class Local_MB_Zaprudnoye
			{
				name="Zaprudnoye Military";
				position[]={4964,12908};
				type="SmallMilitary";
			};
			class Local_MB_Bashnya
			{
				name="Bashnya Military";
				position[]={4107,11660};
				type="MediumMilitary";
			};
			class Local_MB_TriKresta
			{
				name="Tri Kresta Military";
				position[]={324,9324};
				type="SmallMilitary";
			};
			class Local_MB_Tikhiyzaliv
			{
				name="Tikhiyzaliv Military";
				position[]={780,1870};
				type="MediumMilitary";
			};
			class Local_MB_PrisonIsland
			{
				name="Prison Island Military";
				position[]={2706,1355};
				type="MediumMilitary";
			};
			class Local_MB_Chernogorsk
			{
				name="Chernogorsk Military";
				position[]={7260,3024};
				type="SmallMilitary";
			};
			class Local_MB_Novoselki
			{
				name="Novoselki Military";
				position[]={5820,3080};
				type="SmallMilitary";
			};
			class Local_MB_Topolka
			{
				name="Topolka Military";
				position[]={10236,4029};
				type="SmallMilitary";
			};
			class Local_MB_Staroye
			{
				name="Staroye Military";
				position[]={10441,5970};
				type="MediumMilitary";
			};
			class Local_MB_Gorka
			{
				name="Gorka Military";
				position[]={10470,9170};
				type="SmallMilitary";
			};
			class Local_MB_Myshkino
			{
				name="Myshkino Military Base";
				position[]={1190,7240};
				type="MediumMilitary";
			};
			class Local_MB_VMC
			{
				name="VMC Military Base";
				position[]={4600,8291};
				type="BigMilitary";
			};
			class Local_MB_Tisy
			{
				name="Tisy Military Base";
				position[]={1390,13840};
				type="BigMilitary";
			};
			class Local_MB_Kamensk
			{
				name="Troitskoe Military Base";
				position[]={7920,14730};
				type="BigMilitary";
			};
			class Local_MB_Zeleno
			{
				name="Zeleno Military";
				position[]={2415.375,5160.6812};
				type="MediumMilitary";
			};
			class Local_MB_Pavlovo
			{
				name="Pavlovo Military Base";
				position[]={2136,3516};
				type="BigMilitary";
			};
			class Local_AF_Balota
			{
				name="Airstrip Balota";
				position[]={5082.0308,2376.3279};
				type="BigMilitary";
			};
			class Local_AF_Krasno
			{
				name="Airstrip Krasnostav";
				position[]={12100.681,12573.896};
				type="MediumMilitary";
			};
			class Local_AF_Vybor
			{
				name="North West Airfield";
				position[]={4428,10179};
				type="BigMilitary";
			};
			class RailroadStation_Elektro
			{
				name="";
				position[]={10309.912,2092.2029};
				type="RailroadStation";
			};
			class RailroadStation_Prigorodki
			{
				name="";
				position[]={8056.0342,3264.908};
				type="RailroadStation";
			};
			class RailroadStation_Cherno
			{
				name="";
				position[]={6540.626,2638.7749};
				type="RailroadStation";
			};
			class RailroadStation_Balota
			{
				name="";
				position[]={4399.2588,2309.397};
				type="RailroadStation";
			};
			class RailroadStation_Komarovo
			{
				name="";
				position[]={3670.667,2397.5249};
				type="RailroadStation";
			};
			class RailroadStation_Kamenka
			{
				name="";
				position[]={1877.9351,2168.5591};
				type="RailroadStation";
			};
			class RailroadStation_Zeleno
			{
				name="";
				position[]={2485.2661,5214.6641};
				type="RailroadStation";
			};
			class RailroadStation_Vavilovo
			{
				name="";
				position[]={1991.5031,11289.488};
				type="RailroadStation";
			};
			class RailroadStation_Novaya
			{
				name="";
				position[]={3534.425,12537.095};
				type="RailroadStation";
			};
			class RailroadStation_Severograd
			{
				name="";
				position[]={7846.5811,12401.488};
				type="RailroadStation";
			};
			class RailroadStation_Novo
			{
				name="";
				position[]={11610.908,14133.702};
				type="RailroadStation";
			};
			class RailroadStation_Svetlo
			{
				name="";
				position[]={13970.473,13486.344};
				type="RailroadStation";
			};
			class RailroadStation_Berezino
			{
				name="";
				position[]={12989.844,10200.673};
				type="RailroadStation";
			};
			class RailroadStation_Solnich
			{
				name="";
				position[]={13146.109,7094.4722};
				type="RailroadStation";
			};
			class RailroadStation_Solnich2
			{
				name="";
				position[]={13356.109,6154.4722};
				type="RailroadStation";
			};
			class RailroadStation_Kamyshovo
			{
				name="";
				position[]={11970.969,3548.7229};
				type="RailroadStation";
			};
			class RailroadStation_Dobroye
			{
				name="";
				position[]={12860.658,15050.221};
				type="RailroadStation";
			};
			class LocalOffice_Novaya
			{
				name="";
				position[]={3469.1721,13135.165};
				type="LocalOffice";
			};
			class LocalOffice_Vybor
			{
				name="";
				position[]={3857.656,8946.1396};
				type="LocalOffice";
			};
			class LocalOffice_Sinys
			{
				name="";
				position[]={1439.08,12011.26};
				type="LocalOffice";
			};
			class LocalOffice_Vavil
			{
				name="";
				position[]={2220.8401,11075.83};
				type="LocalOffice";
			};
			class LocalOffice_Lopat
			{
				name="";
				position[]={2720.72,9978.1396};
				type="LocalOffice";
			};
			class LocalOffice_Pustosh
			{
				name="";
				position[]={3051.4399,7804.6802};
				type="LocalOffice";
			};
			class LocalOffice_Pogorev
			{
				name="";
				position[]={4480.71,6462};
				type="LocalOffice";
			};
			class LocalOffice_Kabanin
			{
				name="";
				position[]={5382,8589};
				type="LocalOffice";
			};
			class LocalOffice_Stary
			{
				name="";
				position[]={5971.0801,7743.3501};
				type="LocalOffice";
			};
			class LocalOffice_Novy
			{
				name="";
				position[]={7166.96,7526.0098};
				type="LocalOffice";
			};
			class LocalOffice_Grishino
			{
				name="";
				position[]={5983,10305};
				type="LocalOffice";
			};
			class LocalOffice_Severo
			{
				name="";
				position[]={7986,12732};
				type="LocalOffice";
			};
			class LocalOffice_Gorka
			{
				name="";
				position[]={9503,8805};
				type="LocalOffice";
			};
			class LocalOffice_Mogi
			{
				name="";
				position[]={7562,5149};
				type="LocalOffice";
			};
			class LocalOffice_Nadez
			{
				name="";
				position[]={5906,4844};
				type="LocalOffice";
			};
			class LocalOffice_Cherno
			{
				name="";
				position[]={6607,2418};
				type="LocalOffice";
			};
			class LocalOffice_Kozlov
			{
				name="";
				position[]={4394,4629};
				type="LocalOffice";
			};
			class LocalOffice_Komar
			{
				name="";
				position[]={3676,2501};
				type="LocalOffice";
			};
			class LocalOffice_Pavlovo
			{
				name="";
				position[]={1655,3851};
				type="LocalOffice";
			};
			class LocalOffice_Zeleno
			{
				name="";
				position[]={2854,5282};
				type="LocalOffice";
			};
			class LocalOffice_Elektro
			{
				name="";
				position[]={10212,2295};
				type="LocalOffice";
			};
			class LocalOffice_Kamys
			{
				name="";
				position[]={12155,3514};
				type="LocalOffice";
			};
			class LocalOffice_Staroye
			{
				name="";
				position[]={10106,5457};
				type="LocalOffice";
			};
			class LocalOffice_Dolina
			{
				name="";
				position[]={11217,6564};
				type="LocalOffice";
			};
			class LocalOffice_Solnich
			{
				name="";
				position[]={13383,6225};
				type="LocalOffice";
			};
			class LocalOffice_Polana
			{
				name="";
				position[]={10676,7989};
				type="LocalOffice";
			};
			class LocalOffice_Berezino
			{
				name="";
				position[]={12272,9474};
				type="LocalOffice";
			};
			class LocalOffice_Krasno
			{
				name="";
				position[]={11257,12211};
				type="LocalOffice";
			};
			class LocalOffice_Svetlo
			{
				name="";
				position[]={14013,13335};
				type="LocalOffice";
			};
			class LocalOffice_Cpol
			{
				name="";
				position[]={12110,13779};
				type="LocalOffice";
			};
			class LocalOffice_Novo
			{
				name="";
				position[]={11564,14742};
				type="LocalOffice";
			};
			class ViewPoint_1
			{
				name="";
				position[]={567,6997};
				type="ViewPoint";
			};
			class ViewPoint_2
			{
				name="";
				position[]={687,6874};
				type="ViewPoint";
			};
			class ViewPoint_3
			{
				name="";
				position[]={724,6510};
				type="ViewPoint";
			};
			class ViewPoint_4
			{
				name="";
				position[]={298,3460};
				type="ViewPoint";
			};
			class ViewPoint_5
			{
				name="";
				position[]={172,3412};
				type="ViewPoint";
			};
			class ViewPoint_6
			{
				name="";
				position[]={258,3283};
				type="ViewPoint";
			};
			class ViewPoint_7
			{
				name="";
				position[]={322,2697};
				type="ViewPoint";
			};
			class ViewPoint_8
			{
				name="";
				position[]={1671,2211};
				type="ViewPoint";
			};
			class ViewPoint_9
			{
				name="";
				position[]={2915,3995};
				type="ViewPoint";
			};
			class ViewPoint_10
			{
				name="";
				position[]={3371,4551};
				type="ViewPoint";
			};
			class ViewPoint_11
			{
				name="";
				position[]={414,9574};
				type="ViewPoint";
			};
			class ViewPoint_12
			{
				name="";
				position[]={241,11867};
				type="ViewPoint";
			};
			class ViewPoint_13
			{
				name="";
				position[]={1153,12411};
				type="ViewPoint";
			};
			class ViewPoint_14
			{
				name="";
				position[]={2320,13650};
				type="ViewPoint";
			};
			class ViewPoint_15
			{
				name="";
				position[]={4084,14570};
				type="ViewPoint";
			};
			class ViewPoint_16
			{
				name="";
				position[]={4445,15045};
				type="ViewPoint";
			};
			class ViewPoint_17
			{
				name="";
				position[]={4926,14820};
				type="ViewPoint";
			};
			class ViewPoint_18
			{
				name="";
				position[]={5172,14841};
				type="ViewPoint";
			};
			class ViewPoint_19
			{
				name="";
				position[]={5345,14873};
				type="ViewPoint";
			};
			class ViewPoint_20
			{
				name="";
				position[]={5480,14822};
				type="ViewPoint";
			};
			class ViewPoint_21
			{
				name="";
				position[]={5692,14771};
				type="ViewPoint";
			};
			class ViewPoint_22
			{
				name="";
				position[]={5896,14747};
				type="ViewPoint";
			};
			class ViewPoint_23
			{
				name="";
				position[]={6246,14707};
				type="ViewPoint";
			};
			class ViewPoint_24
			{
				name="";
				position[]={6434,14816};
				type="ViewPoint";
			};
			class ViewPoint_25
			{
				name="";
				position[]={6707,13961};
				type="ViewPoint";
			};
			class ViewPoint_26
			{
				name="";
				position[]={6861,14115};
				type="ViewPoint";
			};
			class ViewPoint_27
			{
				name="";
				position[]={8710,14200};
				type="ViewPoint";
			};
			class ViewPoint_28
			{
				name="";
				position[]={10822,14651};
				type="ViewPoint";
			};
			class ViewPoint_29
			{
				name="";
				position[]={11369,15037};
				type="ViewPoint";
			};
			class ViewPoint_30
			{
				name="";
				position[]={13604,14632};
				type="ViewPoint";
			};
			class ViewPoint_31
			{
				name="";
				position[]={13877,14703};
				type="ViewPoint";
			};
			class ViewPoint_32
			{
				name="";
				position[]={14140,15244};
				type="ViewPoint";
			};
			class ViewPoint_33
			{
				name="";
				position[]={13840,13833};
				type="ViewPoint";
			};
			class ViewPoint_34
			{
				name="";
				position[]={14075,13889};
				type="ViewPoint";
			};
			class ViewPoint_35
			{
				name="";
				position[]={14895,13611};
				type="ViewPoint";
			};
			class ViewPoint_36
			{
				name="";
				position[]={14828,13759};
				type="ViewPoint";
			};
			class ViewPoint_37
			{
				name="";
				position[]={11982,13388};
				type="ViewPoint";
			};
			class ViewPoint_38
			{
				name="";
				position[]={11690,13511};
				type="ViewPoint";
			};
			class ViewPoint_39
			{
				name="";
				position[]={11473,13580};
				type="ViewPoint";
			};
			class ViewPoint_40
			{
				name="";
				position[]={11376,13661};
				type="ViewPoint";
			};
			class ViewPoint_41
			{
				name="";
				position[]={13937,11250};
				type="ViewPoint";
			};
			class ViewPoint_42
			{
				name="";
				position[]={11481,8429};
				type="ViewPoint";
			};
			class ViewPoint_43
			{
				name="";
				position[]={11574,7743};
				type="ViewPoint";
			};
			class ViewPoint_44
			{
				name="";
				position[]={12123,6997};
				type="ViewPoint";
			};
			class ViewPoint_45
			{
				name="";
				position[]={12980,5967};
				type="ViewPoint";
			};
			class ViewPoint_46
			{
				name="";
				position[]={12352,4519};
				type="ViewPoint";
			};
			class ViewPoint_47
			{
				name="";
				position[]={12304,3976};
				type="ViewPoint";
			};
			class ViewPoint_48
			{
				name="";
				position[]={10584,2867};
				type="ViewPoint";
			};
			class ViewPoint_49
			{
				name="";
				position[]={8767,2959};
				type="ViewPoint";
			};
			class ViewPoint_50
			{
				name="";
				position[]={8811,2850};
				type="ViewPoint";
			};
			class ViewPoint_51
			{
				name="";
				position[]={8547,3872};
				type="ViewPoint";
			};
			class ViewPoint_52
			{
				name="";
				position[]={9029,4380};
				type="ViewPoint";
			};
			class ViewPoint_53
			{
				name="";
				position[]={10741,4180};
				type="ViewPoint";
			};
			class ViewPoint_54
			{
				name="";
				position[]={6897,11648};
				type="ViewPoint";
			};
			class ViewPoint_55
			{
				name="";
				position[]={5633,6885};
				type="ViewPoint";
			};
			class ViewPoint_56
			{
				name="";
				position[]={1456,9611};
				type="ViewPoint";
			};
			class ViewPoint_57
			{
				name="";
				position[]={1941,6768};
				type="ViewPoint";
			};
			class FeedShack_1
			{
				name="";
				position[]={80.255409,4422.1587};
				type="FeedShack";
			};
			class FeedShack_2
			{
				name="";
				position[]={80.250587,10449.115};
				type="FeedShack";
			};
			class FeedShack_3
			{
				name="";
				position[]={151.03528,3655.1455};
				type="FeedShack";
			};
			class FeedShack_4
			{
				name="";
				position[]={178.26767,7858.5659};
				type="FeedShack";
			};
			class FeedShack_5
			{
				name="";
				position[]={139.80533,14357.006};
				type="FeedShack";
			};
			class FeedShack_6
			{
				name="";
				position[]={209.61076,11768.07};
				type="FeedShack";
			};
			class DeerStand_1
			{
				name="";
				position[]={196.36861,11938.012};
				type="DeerStand";
			};
			class DeerStand_2
			{
				name="";
				position[]={258.46045,4340.9609};
				type="DeerStand";
			};
			class DeerStand_3
			{
				name="";
				position[]={275.75815,5026.4751};
				type="DeerStand";
			};
			class FeedShack_7
			{
				name="";
				position[]={271.58389,5607.2637};
				type="FeedShack";
			};
			class DeerStand_4
			{
				name="";
				position[]={284.28363,9767.2314};
				type="DeerStand";
			};
			class FeedShack_8
			{
				name="";
				position[]={320.44788,12537.249};
				type="FeedShack";
			};
			class DeerStand_5
			{
				name="";
				position[]={348.41824,12972.146};
				type="DeerStand";
			};
			class DeerStand_6
			{
				name="";
				position[]={386.4899,3859.374};
				type="DeerStand";
			};
			class FeedShack_9
			{
				name="";
				position[]={398.86691,4056.4565};
				type="FeedShack";
			};
			class DeerStand_7
			{
				name="";
				position[]={366.03699,6427.5757};
				type="DeerStand";
			};
			class DeerStand_8
			{
				name="";
				position[]={469.45618,2043.0906};
				type="DeerStand";
			};
			class FeedShack_10
			{
				name="";
				position[]={422.99664,9848.8174};
				type="FeedShack";
			};
			class FeedShack_11
			{
				name="";
				position[]={456.32657,13249.15};
				type="FeedShack";
			};
			class DeerStand_9
			{
				name="";
				position[]={537.17035,9312.6006};
				type="DeerStand";
			};
			class DeerStand_10
			{
				name="";
				position[]={532.37878,10757.907};
				type="DeerStand";
			};
			class FeedShack_12
			{
				name="";
				position[]={597.51147,6747.6616};
				type="FeedShack";
			};
			class DeerStand_11
			{
				name="";
				position[]={555.71411,7361.3662};
				type="DeerStand";
			};
			class FeedShack_13
			{
				name="";
				position[]={576.44434,8142.4214};
				type="FeedShack";
			};
			class FeedShack_14
			{
				name="";
				position[]={647.71851,2313.1157};
				type="FeedShack";
			};
			class DeerStand_12
			{
				name="";
				position[]={628.04187,7189.3936};
				type="DeerStand";
			};
			class DeerStand_13
			{
				name="";
				position[]={677.39996,2779.0366};
				type="DeerStand";
			};
			class DeerStand_14
			{
				name="";
				position[]={766.84515,6010.1445};
				type="DeerStand";
			};
			class FeedShack_15
			{
				name="";
				position[]={820.72174,3153.1565};
				type="FeedShack";
			};
			class DeerStand_15
			{
				name="";
				position[]={795.36981,4727.4199};
				type="DeerStand";
			};
			class FeedShack_16
			{
				name="";
				position[]={798.05524,4781.3442};
				type="FeedShack";
			};
			class DeerStand_16
			{
				name="";
				position[]={831.70697,6977.1353};
				type="DeerStand";
			};
			class FeedShack_17
			{
				name="";
				position[]={807.18408,7155.584};
				type="FeedShack";
			};
			class DeerStand_17
			{
				name="";
				position[]={786.58374,8676.4277};
				type="DeerStand";
			};
			class DeerStand_18
			{
				name="";
				position[]={804.83423,10798.019};
				type="DeerStand";
			};
			class DeerStand_19
			{
				name="";
				position[]={884.0929,4147.7661};
				type="DeerStand";
			};
			class DeerStand_20
			{
				name="";
				position[]={873.90405,5302.4067};
				type="DeerStand";
			};
			class FeedShack_18
			{
				name="";
				position[]={877.2627,6184.3823};
				type="FeedShack";
			};
			class FeedShack_19
			{
				name="";
				position[]={962.60382,6855.6343};
				type="FeedShack";
			};
			class DeerStand_21
			{
				name="";
				position[]={1056.5652,11242.221};
				type="DeerStand";
			};
			class DeerStand_22
			{
				name="";
				position[]={1032.9987,12424.235};
				type="DeerStand";
			};
			class DeerStand_23
			{
				name="";
				position[]={1100.016,3585.5544};
				type="DeerStand";
			};
			class DeerStand_24
			{
				name="";
				position[]={1276.8246,7723.9961};
				type="DeerStand";
			};
			class FeedShack_20
			{
				name="";
				position[]={1292.369,7988.3013};
				type="FeedShack";
			};
			class DeerStand_25
			{
				name="";
				position[]={1270.7472,10175.732};
				type="DeerStand";
			};
			class FeedShack_21
			{
				name="";
				position[]={1354.8329,2457.8843};
				type="FeedShack";
			};
			class FeedShack_22
			{
				name="";
				position[]={1371.5018,6786.6357};
				type="FeedShack";
			};
			class DeerStand_26
			{
				name="";
				position[]={1350.6836,9858.7871};
				type="DeerStand";
			};
			class FeedShack_23
			{
				name="";
				position[]={1386.1371,15012.595};
				type="FeedShack";
			};
			class DeerStand_27
			{
				name="";
				position[]={1491.4777,4759.6895};
				type="DeerStand";
			};
			class DeerStand_28
			{
				name="";
				position[]={1443.8607,6559.9517};
				type="DeerStand";
			};
			class DeerStand_29
			{
				name="";
				position[]={1487.2561,8567.6211};
				type="DeerStand";
			};
			class DeerStand_30
			{
				name="";
				position[]={1460.335,14897.563};
				type="DeerStand";
			};
			class DeerStand_31
			{
				name="";
				position[]={1581.8295,10646.833};
				type="DeerStand";
			};
			class DeerStand_32
			{
				name="";
				position[]={1676.6563,4699.4395};
				type="DeerStand";
			};
			class FeedShack_24
			{
				name="";
				position[]={1667.447,4932.4443};
				type="FeedShack";
			};
			class DeerStand_33
			{
				name="";
				position[]={1740.9531,3235.3611};
				type="DeerStand";
			};
			class FeedShack_25
			{
				name="";
				position[]={1797.986,8055.3706};
				type="FeedShack";
			};
			class FeedShack_26
			{
				name="";
				position[]={1770.8844,8434.2607};
				type="FeedShack";
			};
			class DeerStand_34
			{
				name="";
				position[]={1805.2231,11432.715};
				type="DeerStand";
			};
			class DeerStand_35
			{
				name="";
				position[]={1918.3335,9409.0869};
				type="DeerStand";
			};
			class DeerStand_36
			{
				name="";
				position[]={2029.4265,3048.3269};
				type="DeerStand";
			};
			class DeerStand_37
			{
				name="";
				position[]={2022.2919,6598.0762};
				type="DeerStand";
			};
			class FeedShack_27
			{
				name="";
				position[]={1987.264,12965.86};
				type="FeedShack";
			};
			class FeedShack_28
			{
				name="";
				position[]={2057.731,6277.8369};
				type="FeedShack";
			};
			class DeerStand_38
			{
				name="";
				position[]={2092.7781,14762.104};
				type="DeerStand";
			};
			class DeerStand_39
			{
				name="";
				position[]={2112.3901,2630.3486};
				type="DeerStand";
			};
			class DeerStand_40
			{
				name="";
				position[]={2192.4531,7547.8062};
				type="DeerStand";
			};
			class DeerStand_41
			{
				name="";
				position[]={2166.572,12707.179};
				type="DeerStand";
			};
			class FeedShack_29
			{
				name="";
				position[]={2171.2529,13767.172};
				type="FeedShack";
			};
			class DeerStand_42
			{
				name="";
				position[]={2271.8198,4283.4897};
				type="DeerStand";
			};
			class DeerStand_43
			{
				name="";
				position[]={2276.8843,5970.9692};
				type="DeerStand";
			};
			class DeerStand_44
			{
				name="";
				position[]={2298.4988,8493.9678};
				type="DeerStand";
			};
			class FeedShack_30
			{
				name="";
				position[]={2394.4619,14753.762};
				type="FeedShack";
			};
			class DeerStand_45
			{
				name="";
				position[]={2491.1238,6882.457};
				type="DeerStand";
			};
			class DeerStand_46
			{
				name="";
				position[]={2489.6299,7680.27};
				type="DeerStand";
			};
			class FeedShack_31
			{
				name="";
				position[]={2501.2539,11130.858};
				type="FeedShack";
			};
			class FeedShack_32
			{
				name="";
				position[]={2543.6438,7593.2686};
				type="FeedShack";
			};
			class FeedShack_33
			{
				name="";
				position[]={2563.8984,9221.7764};
				type="FeedShack";
			};
			class DeerStand_47
			{
				name="";
				position[]={2538.3708,9381.5479};
				type="DeerStand";
			};
			class DeerStand_48
			{
				name="";
				position[]={2577.1372,13904.271};
				type="DeerStand";
			};
			class DeerStand_49
			{
				name="";
				position[]={2626.6638,11336.511};
				type="DeerStand";
			};
			class DeerStand_50
			{
				name="";
				position[]={2737.9558,3598.0142};
				type="DeerStand";
			};
			class FeedShack_34
			{
				name="";
				position[]={2747.5425,4226.2866};
				type="FeedShack";
			};
			class FeedShack_35
			{
				name="";
				position[]={2813.127,2482.8994};
				type="FeedShack";
			};
			class FeedShack_36
			{
				name="";
				position[]={2974.1953,13591.542};
				type="FeedShack";
			};
			class DeerStand_51
			{
				name="";
				position[]={2943.9775,14412.018};
				type="DeerStand";
			};
			class DeerStand_52
			{
				name="";
				position[]={3118.24,2397.1475};
				type="DeerStand";
			};
			class FeedShack_37
			{
				name="";
				position[]={3063.5647,6581.6528};
				type="FeedShack";
			};
			class DeerStand_53
			{
				name="";
				position[]={3228.1648,3600.8669};
				type="DeerStand";
			};
			class DeerStand_54
			{
				name="";
				position[]={3214.3057,6287.8037};
				type="DeerStand";
			};
			class DeerStand_55
			{
				name="";
				position[]={3199.4009,11591.539};
				type="DeerStand";
			};
			class DeerStand_56
			{
				name="";
				position[]={3282.5129,5767.6304};
				type="DeerStand";
			};
			class FeedShack_38
			{
				name="";
				position[]={3273.1233,11624.183};
				type="FeedShack";
			};
			class DeerStand_57
			{
				name="";
				position[]={3360.8796,4284.3169};
				type="DeerStand";
			};
			class DeerStand_58
			{
				name="";
				position[]={3418.3276,10188.814};
				type="DeerStand";
			};
			class DeerStand_59
			{
				name="";
				position[]={3374.0562,11019.997};
				type="DeerStand";
			};
			class DeerStand_60
			{
				name="";
				position[]={3423.884,8144.5952};
				type="DeerStand";
			};
			class DeerStand_61
			{
				name="";
				position[]={3443.3547,12136.321};
				type="DeerStand";
			};
			class DeerStand_62
			{
				name="";
				position[]={3654.3357,6260.98};
				type="DeerStand";
			};
			class DeerStand_63
			{
				name="";
				position[]={3603.959,9367.5703};
				type="DeerStand";
			};
			class FeedShack_39
			{
				name="";
				position[]={3614.8123,10572.549};
				type="FeedShack";
			};
			class DeerStand_64
			{
				name="";
				position[]={3746.5769,2979.0566};
				type="DeerStand";
			};
			class DeerStand_65
			{
				name="";
				position[]={3818.0735,14120.452};
				type="DeerStand";
			};
			class DeerStand_66
			{
				name="";
				position[]={3871.459,4434.8315};
				type="DeerStand";
			};
			class DeerStand_67
			{
				name="";
				position[]={3965.7505,12130.793};
				type="DeerStand";
			};
			class DeerStand_68
			{
				name="";
				position[]={4079.5762,3865.7629};
				type="DeerStand";
			};
			class DeerStand_69
			{
				name="";
				position[]={4081.3357,6016.4468};
				type="DeerStand";
			};
			class DeerStand_70
			{
				name="";
				position[]={4111.5591,15170.728};
				type="DeerStand";
			};
			class FeedShack_40
			{
				name="";
				position[]={4145.2949,3819.48};
				type="FeedShack";
			};
			class DeerStand_71
			{
				name="";
				position[]={4221.7183,7087.9824};
				type="DeerStand";
			};
			class DeerStand_72
			{
				name="";
				position[]={4315.5166,14287.283};
				type="DeerStand";
			};
			class DeerStand_73
			{
				name="";
				position[]={4351.9849,4973.5498};
				type="DeerStand";
			};
			class FeedShack_41
			{
				name="";
				position[]={4364.2241,14671.948};
				type="FeedShack";
			};
			class DeerStand_74
			{
				name="";
				position[]={4393.4199,8058.584};
				type="DeerStand";
			};
			class FeedShack_42
			{
				name="";
				position[]={4389.1113,11896.748};
				type="FeedShack";
			};
			class DeerStand_75
			{
				name="";
				position[]={4414.0874,12237.998};
				type="DeerStand";
			};
			class FeedShack_43
			{
				name="";
				position[]={4444.7344,5074.0454};
				type="FeedShack";
			};
			class FeedShack_44
			{
				name="";
				position[]={4544.9199,3130.6079};
				type="FeedShack";
			};
			class DeerStand_76
			{
				name="";
				position[]={4719.4917,14976.774};
				type="DeerStand";
			};
			class DeerStand_77
			{
				name="";
				position[]={4700.0474,15234.973};
				type="DeerStand";
			};
			class DeerStand_78
			{
				name="";
				position[]={4826.6133,3318.2087};
				type="DeerStand";
			};
			class FeedShack_45
			{
				name="";
				position[]={4839.2275,3845.3948};
				type="FeedShack";
			};
			class DeerStand_79
			{
				name="";
				position[]={4838.2764,4611.082};
				type="DeerStand";
			};
			class DeerStand_80
			{
				name="";
				position[]={4852.2124,13584.429};
				type="DeerStand";
			};
			class DeerStand_81
			{
				name="";
				position[]={4882.7432,8150.2671};
				type="DeerStand";
			};
			class DeerStand_82
			{
				name="";
				position[]={4880.1216,12475.938};
				type="DeerStand";
			};
			class DeerStand_83
			{
				name="";
				position[]={4951.2451,6380.9751};
				type="DeerStand";
			};
			class DeerStand_84
			{
				name="";
				position[]={4998.5649,5068.7603};
				type="DeerStand";
			};
			class FeedShack_46
			{
				name="";
				position[]={5026.8262,11275.82};
				type="FeedShack";
			};
			class DeerStand_85
			{
				name="";
				position[]={5071.2817,6279.5142};
				type="DeerStand";
			};
			class FeedShack_47
			{
				name="";
				position[]={5097.8359,6386.3994};
				type="FeedShack";
			};
			class DeerStand_86
			{
				name="";
				position[]={5209.1191,4085.2639};
				type="DeerStand";
			};
			class FeedShack_48
			{
				name="";
				position[]={5263.2212,4857.9331};
				type="FeedShack";
			};
			class DeerStand_87
			{
				name="";
				position[]={5276.7388,11899.777};
				type="DeerStand";
			};
			class DeerStand_88
			{
				name="";
				position[]={5329.6665,5795.3218};
				type="DeerStand";
			};
			class DeerStand_89
			{
				name="";
				position[]={5338.4336,7360.7549};
				type="DeerStand";
			};
			class FeedShack_49
			{
				name="";
				position[]={5298.0879,13590.939};
				type="FeedShack";
			};
			class FeedShack_50
			{
				name="";
				position[]={5346.1729,7893.0195};
				type="FeedShack";
			};
			class DeerStand_90
			{
				name="";
				position[]={5466.6196,7888.1421};
				type="DeerStand";
			};
			class DeerStand_91
			{
				name="";
				position[]={5543.3306,9563.0186};
				type="DeerStand";
			};
			class DeerStand_92
			{
				name="";
				position[]={5535.7593,11251.677};
				type="DeerStand";
			};
			class DeerStand_93
			{
				name="";
				position[]={5520.5664,13834.319};
				type="DeerStand";
			};
			class DeerStand_94
			{
				name="";
				position[]={5564.5864,15084.688};
				type="DeerStand";
			};
			class DeerStand_95
			{
				name="";
				position[]={5669.7515,6843.3364};
				type="DeerStand";
			};
			class FeedShack_51
			{
				name="";
				position[]={5682.5864,14211.78};
				type="FeedShack";
			};
			class DeerStand_96
			{
				name="";
				position[]={5646.1455,14309.629};
				type="DeerStand";
			};
			class DeerStand_97
			{
				name="";
				position[]={5869.3477,4303.3174};
				type="DeerStand";
			};
			class DeerStand_98
			{
				name="";
				position[]={5921.1665,3466.4026};
				type="DeerStand";
			};
			class DeerStand_99
			{
				name="";
				position[]={6085.9429,5033.9873};
				type="DeerStand";
			};
			class DeerStand_100
			{
				name="";
				position[]={6115.564,6129.0957};
				type="DeerStand";
			};
			class FeedShack_52
			{
				name="";
				position[]={6155.0483,5688.3359};
				type="FeedShack";
			};
			class DeerStand_101
			{
				name="";
				position[]={6144.7212,13133.608};
				type="DeerStand";
			};
			class FeedShack_53
			{
				name="";
				position[]={6187.6572,4090.8728};
				type="FeedShack";
			};
			class DeerStand_102
			{
				name="";
				position[]={6227.5615,9750.3945};
				type="DeerStand";
			};
			class FeedShack_54
			{
				name="";
				position[]={6204.8628,11372.897};
				type="FeedShack";
			};
			class DeerStand_103
			{
				name="";
				position[]={6213.1357,11360.18};
				type="DeerStand";
			};
			class FeedShack_55
			{
				name="";
				position[]={6220.2695,15337.155};
				type="FeedShack";
			};
			class DeerStand_104
			{
				name="";
				position[]={6296.6714,7822.1729};
				type="DeerStand";
			};
			class DeerStand_105
			{
				name="";
				position[]={6250.1318,12220.539};
				type="DeerStand";
			};
			class DeerStand_106
			{
				name="";
				position[]={6290.2568,14224.72};
				type="DeerStand";
			};
			class DeerStand_107
			{
				name="";
				position[]={6332.4844,7949.2563};
				type="DeerStand";
			};
			class DeerStand_108
			{
				name="";
				position[]={6376.417,6573.8047};
				type="DeerStand";
			};
			class DeerStand_109
			{
				name="";
				position[]={6375.0435,7788.1357};
				type="DeerStand";
			};
			class DeerStand_110
			{
				name="";
				position[]={6434.4253,8473.6191};
				type="DeerStand";
			};
			class DeerStand_111
			{
				name="";
				position[]={6579.5669,4391.3916};
				type="DeerStand";
			};
			class DeerStand_112
			{
				name="";
				position[]={6601.2744,15192.606};
				type="DeerStand";
			};
			class FeedShack_56
			{
				name="";
				position[]={6682.3711,6772.4258};
				type="FeedShack";
			};
			class DeerStand_113
			{
				name="";
				position[]={6696.3599,8429.7676};
				type="DeerStand";
			};
			class FeedShack_57
			{
				name="";
				position[]={6703.7759,13709.309};
				type="FeedShack";
			};
			class DeerStand_114
			{
				name="";
				position[]={6736.1592,5409.0234};
				type="DeerStand";
			};
			class DeerStand_115
			{
				name="";
				position[]={6784.2554,7184.1543};
				type="DeerStand";
			};
			class DeerStand_116
			{
				name="";
				position[]={6798.0098,12984.001};
				type="DeerStand";
			};
			class DeerStand_117
			{
				name="";
				position[]={6815.0908,13603.063};
				type="DeerStand";
			};
			class DeerStand_118
			{
				name="";
				position[]={6856.5483,8004.647};
				type="DeerStand";
			};
			class FeedShack_58
			{
				name="";
				position[]={6889.0293,10862.519};
				type="FeedShack";
			};
			class FeedShack_59
			{
				name="";
				position[]={6860.7798,15051.707};
				type="FeedShack";
			};
			class FeedShack_60
			{
				name="";
				position[]={6997.1904,4549.7202};
				type="FeedShack";
			};
			class DeerStand_119
			{
				name="";
				position[]={7071.3608,9943.9502};
				type="DeerStand";
			};
			class FeedShack_61
			{
				name="";
				position[]={7024.9717,13569.238};
				type="FeedShack";
			};
			class FeedShack_62
			{
				name="";
				position[]={7088.2515,9160.8867};
				type="FeedShack";
			};
			class DeerStand_120
			{
				name="";
				position[]={7139.6689,11959.412};
				type="DeerStand";
			};
			class FeedShack_63
			{
				name="";
				position[]={7101.6167,12359.935};
				type="FeedShack";
			};
			class DeerStand_121
			{
				name="";
				position[]={7180.4019,14000.493};
				type="DeerStand";
			};
			class DeerStand_122
			{
				name="";
				position[]={7235.3066,5952.167};
				type="DeerStand";
			};
			class DeerStand_123
			{
				name="";
				position[]={7236.6719,10704.551};
				type="DeerStand";
			};
			class FeedShack_64
			{
				name="";
				position[]={7428.1021,12732.725};
				type="FeedShack";
			};
			class DeerStand_124
			{
				name="";
				position[]={7463.2983,6770.5938};
				type="DeerStand";
			};
			class FeedShack_65
			{
				name="";
				position[]={7499.9331,11747.946};
				type="FeedShack";
			};
			class DeerStand_125
			{
				name="";
				position[]={7619.3218,5407.147};
				type="DeerStand";
			};
			class FeedShack_66
			{
				name="";
				position[]={7635.4844,6637.4224};
				type="FeedShack";
			};
			class FeedShack_67
			{
				name="";
				position[]={7652.4995,11186.456};
				type="FeedShack";
			};
			class DeerStand_126
			{
				name="";
				position[]={7626.8687,11781.449};
				type="DeerStand";
			};
			class DeerStand_127
			{
				name="";
				position[]={7696.0342,8689.2217};
				type="DeerStand";
			};
			class DeerStand_128
			{
				name="";
				position[]={7926.2153,13360.151};
				type="DeerStand";
			};
			class DeerStand_129
			{
				name="";
				position[]={8035.27,14131.938};
				type="DeerStand";
			};
			class DeerStand_130
			{
				name="";
				position[]={8079.4863,4485.9287};
				type="DeerStand";
			};
			class DeerStand_131
			{
				name="";
				position[]={8078.7222,15145.27};
				type="DeerStand";
			};
			class DeerStand_132
			{
				name="";
				position[]={8143.4941,5039.3291};
				type="DeerStand";
			};
			class DeerStand_133
			{
				name="";
				position[]={8115.2188,5753.8027};
				type="DeerStand";
			};
			class DeerStand_134
			{
				name="";
				position[]={8135.918,6808.0391};
				type="DeerStand";
			};
			class FeedShack_68
			{
				name="";
				position[]={8118.4658,8237.8799};
				type="FeedShack";
			};
			class FeedShack_69
			{
				name="";
				position[]={8171.707,5446.8481};
				type="FeedShack";
			};
			class DeerStand_135
			{
				name="";
				position[]={8194.2266,7708.6558};
				type="DeerStand";
			};
			class FeedShack_70
			{
				name="";
				position[]={8209.6094,14276.988};
				type="FeedShack";
			};
			class DeerStand_136
			{
				name="";
				position[]={8291.7686,9722.3555};
				type="DeerStand";
			};
			class DeerStand_137
			{
				name="";
				position[]={8337.3242,10673.748};
				type="DeerStand";
			};
			class DeerStand_138
			{
				name="";
				position[]={8368.4463,11439.751};
				type="DeerStand";
			};
			class DeerStand_139
			{
				name="";
				position[]={8406.1357,3301.3469};
				type="DeerStand";
			};
			class DeerStand_140
			{
				name="";
				position[]={8404.8975,8159.7939};
				type="DeerStand";
			};
			class FeedShack_71
			{
				name="";
				position[]={8414.3643,10533.499};
				type="FeedShack";
			};
			class DeerStand_141
			{
				name="";
				position[]={8480.4063,6131.1021};
				type="DeerStand";
			};
			class FeedShack_72
			{
				name="";
				position[]={8546.1875,4394.5771};
				type="FeedShack";
			};
			class FeedShack_73
			{
				name="";
				position[]={8633.5693,12096.213};
				type="FeedShack";
			};
			class DeerStand_142
			{
				name="";
				position[]={8676.9893,12304.579};
				type="DeerStand";
			};
			class DeerStand_143
			{
				name="";
				position[]={8659.5146,14297.824};
				type="DeerStand";
			};
			class FeedShack_74
			{
				name="";
				position[]={8701.7842,8780.0869};
				type="FeedShack";
			};
			class DeerStand_144
			{
				name="";
				position[]={8805.1709,10479.631};
				type="DeerStand";
			};
			class DeerStand_145
			{
				name="";
				position[]={8771.0078,15021.982};
				type="DeerStand";
			};
			class DeerStand_146
			{
				name="";
				position[]={8823.1904,4197.7144};
				type="DeerStand";
			};
			class DeerStand_147
			{
				name="";
				position[]={8844.2871,6457.5527};
				type="DeerStand";
			};
			class FeedShack_75
			{
				name="";
				position[]={8855.4512,6983.5767};
				type="FeedShack";
			};
			class DeerStand_148
			{
				name="";
				position[]={8824.8271,7566.7119};
				type="DeerStand";
			};
			class DeerStand_149
			{
				name="";
				position[]={8826.7197,8649.6533};
				type="DeerStand";
			};
			class FeedShack_76
			{
				name="";
				position[]={8841.2813,13877.817};
				type="FeedShack";
			};
			class DeerStand_150
			{
				name="";
				position[]={8895.1729,5322.2866};
				type="DeerStand";
			};
			class FeedShack_77
			{
				name="";
				position[]={9127.6396,3101.7483};
				type="FeedShack";
			};
			class FeedShack_78
			{
				name="";
				position[]={9177.2031,7769.8252};
				type="FeedShack";
			};
			class DeerStand_151
			{
				name="";
				position[]={9163.3779,15016.912};
				type="DeerStand";
			};
			class DeerStand_152
			{
				name="";
				position[]={9186.0938,8289.3418};
				type="DeerStand";
			};
			class DeerStand_153
			{
				name="";
				position[]={9332.7373,3373.7546};
				type="DeerStand";
			};
			class DeerStand_154
			{
				name="";
				position[]={9325.957,7220.4424};
				type="DeerStand";
			};
			class FeedShack_79
			{
				name="";
				position[]={9303.293,12043.563};
				type="FeedShack";
			};
			class FeedShack_80
			{
				name="";
				position[]={9311.4326,15032.32};
				type="FeedShack";
			};
			class FeedShack_81
			{
				name="";
				position[]={9429.6982,5824.3086};
				type="FeedShack";
			};
			class DeerStand_155
			{
				name="";
				position[]={9453.7148,7718.0054};
				type="DeerStand";
			};
			class DeerStand_156
			{
				name="";
				position[]={9535.5244,2705.5317};
				type="DeerStand";
			};
			class DeerStand_157
			{
				name="";
				position[]={9502.707,9051.1484};
				type="DeerStand";
			};
			class DeerStand_158
			{
				name="";
				position[]={9548.3242,12634.149};
				type="DeerStand";
			};
			class DeerStand_159
			{
				name="";
				position[]={9626.6016,6258.8052};
				type="DeerStand";
			};
			class FeedShack_82
			{
				name="";
				position[]={9656.1289,9187.7861};
				type="FeedShack";
			};
			class DeerStand_160
			{
				name="";
				position[]={9665.5117,3669.4807};
				type="DeerStand";
			};
			class DeerStand_161
			{
				name="";
				position[]={9665.8662,8331.416};
				type="DeerStand";
			};
			class DeerStand_162
			{
				name="";
				position[]={9689.2051,14663.023};
				type="DeerStand";
			};
			class DeerStand_163
			{
				name="";
				position[]={9769.4727,2516.2666};
				type="DeerStand";
			};
			class DeerStand_164
			{
				name="";
				position[]={9728.2227,4496.6841};
				type="DeerStand";
			};
			class DeerStand_165
			{
				name="";
				position[]={9763.4219,11714.377};
				type="DeerStand";
			};
			class DeerStand_166
			{
				name="";
				position[]={9832.1641,14217.416};
				type="DeerStand";
			};
			class DeerStand_167
			{
				name="";
				position[]={9811.543,15234.364};
				type="DeerStand";
			};
			class FeedShack_83
			{
				name="";
				position[]={9884.5742,4360.1616};
				type="FeedShack";
			};
			class DeerStand_168
			{
				name="";
				position[]={9855.335,7239.0171};
				type="DeerStand";
			};
			class DeerStand_169
			{
				name="";
				position[]={9864.7119,13456.027};
				type="DeerStand";
			};
			class DeerStand_170
			{
				name="";
				position[]={9945.5059,3192.2944};
				type="DeerStand";
			};
			class DeerStand_171
			{
				name="";
				position[]={9919.7227,5257.3857};
				type="DeerStand";
			};
			class DeerStand_172
			{
				name="";
				position[]={9914.377,9632.334};
				type="DeerStand";
			};
			class FeedShack_84
			{
				name="";
				position[]={9940.5928,15282.305};
				type="FeedShack";
			};
			class DeerStand_173
			{
				name="";
				position[]={9979.7236,8973.8994};
				type="DeerStand";
			};
			class DeerStand_174
			{
				name="";
				position[]={10085.732,6696.1563};
				type="DeerStand";
			};
			class FeedShack_85
			{
				name="";
				position[]={10143.283,6752.4746};
				type="FeedShack";
			};
			class DeerStand_175
			{
				name="";
				position[]={10227.56,8431.2959};
				type="DeerStand";
			};
			class FeedShack_86
			{
				name="";
				position[]={10297.117,7458.8198};
				type="FeedShack";
			};
			class DeerStand_176
			{
				name="";
				position[]={10333.323,10130.713};
				type="DeerStand";
			};
			class DeerStand_177
			{
				name="";
				position[]={10322.301,14695.112};
				type="DeerStand";
			};
			class DeerStand_178
			{
				name="";
				position[]={10383.368,6011.9727};
				type="DeerStand";
			};
			class FeedShack_87
			{
				name="";
				position[]={10482.508,11750.061};
				type="FeedShack";
			};
			class DeerStand_179
			{
				name="";
				position[]={10443.572,12495.661};
				type="DeerStand";
			};
			class FeedShack_88
			{
				name="";
				position[]={10561.745,12798.186};
				type="FeedShack";
			};
			class DeerStand_180
			{
				name="";
				position[]={10663.933,5224.5317};
				type="DeerStand";
			};
			class FeedShack_89
			{
				name="";
				position[]={10643.495,6059.4976};
				type="FeedShack";
			};
			class DeerStand_181
			{
				name="";
				position[]={10635.291,10921.459};
				type="DeerStand";
			};
			class DeerStand_182
			{
				name="";
				position[]={10704.667,5836.6694};
				type="DeerStand";
			};
			class FeedShack_90
			{
				name="";
				position[]={10783.259,3645.5139};
				type="FeedShack";
			};
			class DeerStand_183
			{
				name="";
				position[]={10797.97,13557.957};
				type="DeerStand";
			};
			class DeerStand_184
			{
				name="";
				position[]={10785.374,14904.296};
				type="DeerStand";
			};
			class DeerStand_185
			{
				name="";
				position[]={10855.99,3497.8445};
				type="DeerStand";
			};
			class DeerStand_186
			{
				name="";
				position[]={10858.775,4244.75};
				type="DeerStand";
			};
			class FeedShack_91
			{
				name="";
				position[]={10877.515,4134.3311};
				type="FeedShack";
			};
			class DeerStand_187
			{
				name="";
				position[]={10898.419,6443.584};
				type="DeerStand";
			};
			class FeedShack_92
			{
				name="";
				position[]={10873.353,10268.762};
				type="FeedShack";
			};
			class FeedShack_93
			{
				name="";
				position[]={10886.481,14709.837};
				type="FeedShack";
			};
			class FeedShack_94
			{
				name="";
				position[]={10967.703,8591.668};
				type="FeedShack";
			};
			class DeerStand_188
			{
				name="";
				position[]={11201.738,7260.8799};
				type="DeerStand";
			};
			class DeerStand_189
			{
				name="";
				position[]={11160.273,9590.6836};
				type="DeerStand";
			};
			class FeedShack_95
			{
				name="";
				position[]={11167.938,9848.0371};
				type="FeedShack";
			};
			class DeerStand_190
			{
				name="";
				position[]={11237.152,5624.0303};
				type="DeerStand";
			};
			class FeedShack_96
			{
				name="";
				position[]={11234.225,13647.42};
				type="FeedShack";
			};
			class DeerStand_191
			{
				name="";
				position[]={11326.162,8057.8008};
				type="DeerStand";
			};
			class DeerStand_192
			{
				name="";
				position[]={11339.577,9193.6475};
				type="DeerStand";
			};
			class DeerStand_193
			{
				name="";
				position[]={11358.841,10188.304};
				type="DeerStand";
			};
			class FeedShack_97
			{
				name="";
				position[]={11364.682,11773.546};
				type="FeedShack";
			};
			class DeerStand_194
			{
				name="";
				position[]={11448.269,5127.8374};
				type="DeerStand";
			};
			class FeedShack_98
			{
				name="";
				position[]={11575.989,4676.3047};
				type="FeedShack";
			};
			class DeerStand_195
			{
				name="";
				position[]={11535.92,13324.779};
				type="DeerStand";
			};
			class DeerStand_196
			{
				name="";
				position[]={11609.092,3733.312};
				type="DeerStand";
			};
			class DeerStand_197
			{
				name="";
				position[]={11687.975,6963.6182};
				type="DeerStand";
			};
			class DeerStand_198
			{
				name="";
				position[]={11707.245,4435.0371};
				type="DeerStand";
			};
			class FeedShack_99
			{
				name="";
				position[]={11756.563,5805.4707};
				type="FeedShack";
			};
			class DeerStand_199
			{
				name="";
				position[]={11750.498,6317.5679};
				type="DeerStand";
			};
			class FeedShack_100
			{
				name="";
				position[]={11733.57,7100.3438};
				type="FeedShack";
			};
			class FeedShack_101
			{
				name="";
				position[]={11816.871,4013.4404};
				type="FeedShack";
			};
			class DeerStand_200
			{
				name="";
				position[]={11819.041,8417.5215};
				type="DeerStand";
			};
			class DeerStand_201
			{
				name="";
				position[]={11868.572,15252.762};
				type="DeerStand";
			};
			class DeerStand_202
			{
				name="";
				position[]={11887.835,9545.0488};
				type="DeerStand";
			};
			class DeerStand_203
			{
				name="";
				position[]={11942.659,10835.888};
				type="DeerStand";
			};
			class FeedShack_102
			{
				name="";
				position[]={12073.034,5111.582};
				type="FeedShack";
			};
			class FeedShack_103
			{
				name="";
				position[]={12130.356,5451.7837};
				type="FeedShack";
			};
			class FeedShack_104
			{
				name="";
				position[]={12128.536,11189.932};
				type="FeedShack";
			};
			class FeedShack_105
			{
				name="";
				position[]={12178.15,14802.256};
				type="FeedShack";
			};
			class DeerStand_204
			{
				name="";
				position[]={12251.652,5433.6763};
				type="DeerStand";
			};
			class DeerStand_205
			{
				name="";
				position[]={12263.446,8735.8125};
				type="DeerStand";
			};
			class DeerStand_206
			{
				name="";
				position[]={12289.344,11441.019};
				type="DeerStand";
			};
			class DeerStand_207
			{
				name="";
				position[]={12345.381,6924.6816};
				type="DeerStand";
			};
			class FeedShack_106
			{
				name="";
				position[]={12378.531,7740.5728};
				type="FeedShack";
			};
			class DeerStand_208
			{
				name="";
				position[]={12372.089,15202.404};
				type="DeerStand";
			};
			class DeerStand_209
			{
				name="";
				position[]={12463.246,4638.8706};
				type="DeerStand";
			};
			class DeerStand_210
			{
				name="";
				position[]={12582.753,7388.1387};
				type="DeerStand";
			};
			class FeedShack_107
			{
				name="";
				position[]={12619.967,6673.7549};
				type="FeedShack";
			};
			class DeerStand_211
			{
				name="";
				position[]={12635.896,12804.073};
				type="DeerStand";
			};
			class FeedShack_108
			{
				name="";
				position[]={12624.723,14528.346};
				type="FeedShack";
			};
			class DeerStand_212
			{
				name="";
				position[]={12719.811,6323.478};
				type="DeerStand";
			};
			class DeerStand_213
			{
				name="";
				position[]={12728.171,5133.4761};
				type="DeerStand";
			};
			class DeerStand_214
			{
				name="";
				position[]={12761.925,7721.4668};
				type="DeerStand";
			};
			class FeedShack_109
			{
				name="";
				position[]={12818.688,13117.197};
				type="FeedShack";
			};
			class DeerStand_215
			{
				name="";
				position[]={12867.194,10430.961};
				type="DeerStand";
			};
			class FeedShack_110
			{
				name="";
				position[]={12890.481,10460.096};
				type="FeedShack";
			};
			class DeerStand_216
			{
				name="";
				position[]={12935.91,12241.822};
				type="DeerStand";
			};
			class DeerStand_217
			{
				name="";
				position[]={13254.438,12791.209};
				type="DeerStand";
			};
			class DeerStand_218
			{
				name="";
				position[]={13200.934,13232.412};
				type="DeerStand";
			};
			class FeedShack_111
			{
				name="";
				position[]={13331.621,5062.0518};
				type="FeedShack";
			};
			class DeerStand_219
			{
				name="";
				position[]={13353.468,15078.26};
				type="DeerStand";
			};
			class FeedShack_112
			{
				name="";
				position[]={13381.251,12246.167};
				type="FeedShack";
			};
			class DeerStand_220
			{
				name="";
				position[]={13452.053,11908.127};
				type="DeerStand";
			};
			class FeedShack_113
			{
				name="";
				position[]={13462.885,12455.709};
				type="FeedShack";
			};
			class DeerStand_221
			{
				name="";
				position[]={13616.199,3089.4629};
				type="DeerStand";
			};
			class DeerStand_222
			{
				name="";
				position[]={13646.509,14477.601};
				type="DeerStand";
			};
			class FeedShack_114
			{
				name="";
				position[]={13746.811,14415.497};
				type="FeedShack";
			};
			class DeerStand_223
			{
				name="";
				position[]={13953.609,14845.188};
				type="DeerStand";
			};
			class DeerStand_224
			{
				name="";
				position[]={14386.711,15063.619};
				type="DeerStand";
			};
			class FeedShack_115
			{
				name="";
				position[]={14422.507,14289.826};
				type="FeedShack";
			};
			class FeedShack_116
			{
				name="";
				position[]={14645.491,14643.557};
				type="FeedShack";
			};
			class DeerStand_225
			{
				name="";
				position[]={14844.645,14073.151};
				type="DeerStand";
			};
			class FeedShack_117
			{
				name="";
				position[]={14917.346,14257.615};
				type="FeedShack";
			};
			class DeerStand_226
			{
				name="";
				position[]={15134.531,14788.876};
				type="DeerStand";
			};
		};
	};
};
