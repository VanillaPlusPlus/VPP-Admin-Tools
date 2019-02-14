class AdvancedLoadouts extends VPPModuleManager
{
	//=============Mod Tunables=============
	protected bool m_SpawnArmed	= true;  //Spawn fresh spawns with a pistol, weapon types can be changed in 'DayZSurvival.c' All the way at the bottom in fucntion 'StartingEquipSetup'
	//======================================

	ref JsonSerializer m_Serializer = new JsonSerializer;

	string m_RandomLoadoutsPath = "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\LoadOuts\\RandomlyGenerated\\";
	ref TStringArray LoadoutCatagories = {"Bags","Gloves","Vests","Tops","Pants","Boots","HeadGear","Face"}; //Add any new catagories here, make sure the name matches everywhere used including file
	ref map<string,string> PoweredOptics; //Type of optics, type of battery

	ref array<string> Bags;
	ref array<string> Gloves;
	ref array<string> Vests;
	ref array<string> Tops;
	ref array<string> Pants;
	ref array<string> Boots;
	ref array<string> HeadGear;
	ref array<string> Face;

	void AdvancedLoadouts( DayZSurvival m_Mission )
	{
		GetRPCManager().AddRPC( "RPC_GetLoadouts", "GetLoadouts", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_CheckStaticLoadouts", "CheckStaticLoadouts", this, SingeplayerExecutionType.Server );

		Bags     = new array<string>;
		Gloves   = new array<string>;
		Vests    = new array<string>;
		Tops     = new array<string>;
		Pants    = new array<string>;
		Boots    = new array<string>;
		HeadGear = new array<string>;
		Face     = new array<string>;

		//Make Defualt LD is none exist
		array<string> DefualtItems = new array<string>;
		array<string> LoadoutDirectorys = new array<string>;
		FindFileHandle FileSearch;
		FileHandle LoadOutDir;
		string MainDirectoy = "$profile:Static_Loadouts";

		DefualtItems.Insert("Rag");
		DefualtItems.Insert("jeans_black");
		DefualtItems.Insert("tshirt_black");
		DefualtItems.Insert("athleticshoes_black");

		//Search and compile loadouts
		string FolderName;
		int 	file_attr;
		int		flags;

		if (FileExist(MainDirectoy))
		{
			string m_Path = "$profile:/Static_Loadouts";
			string path_find_pattern = m_Path + "/*";

			FileSearch = FindFile(path_find_pattern, FolderName, file_attr, flags);

			bool found = true;
			while (found)
			{
				if ( file_attr & FileAttr.DIRECTORY )
				{
					LoadoutDirectorys.Insert(FolderName);
				}
				
				found = FindNextFile(FileSearch, FolderName, file_attr);
			}
			if (LoadoutDirectorys.Count() <= 0)
			{
				//ExportInventoryByClassName(DefualtItems);
			}
			CloseFindFile(FileSearch);
		}
		else
		{
			//ExportInventoryByClassName(DefualtItems);
		}
	}

	void GetLoadouts( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if (type == CallType.Server)
        {
			ref Param1< ref map<string, TStringArray> > map_param;
			ref map<string, TStringArray> m_LootMap = new map<string, TStringArray>;
			//Construct map
			m_LootMap.Insert("Bags",Bags);
			m_LootMap.Insert("Gloves",Gloves);
			m_LootMap.Insert("Vests",Vests);
			m_LootMap.Insert("Tops",Tops);
			m_LootMap.Insert("Pants",Pants);
			m_LootMap.Insert("Boots",Boots);
			m_LootMap.Insert("HeadGear",HeadGear);
			m_LootMap.Insert("Face",Face);

			map_param = new Param1< ref map<string, TStringArray>>(m_LootMap);
			GetRPCManager().SendRPC( "RPC_ItemsHandle", "ItemsHandle", map_param, true, sender );
		}
	}

	bool CheckTunables(string type)
	{
		switch(type)
		{
			case "SpawnArmed":
			return m_SpawnArmed;
			break;
		}
		return false;
	}

	override void Init()
	{
		Print("AdvancedLoadouts:: Init!");

		PoweredOptics = new map<string,string>;
		PoweredOptics.Insert("m4_carryhandleoptic","");
		PoweredOptics.Insert("buisoptic","");
		PoweredOptics.Insert("M68Optic","Battery9V");
		PoweredOptics.Insert("m4_t3nrdsoptic","Battery9V");
		PoweredOptics.Insert("fnp45_mrdsoptic","Battery9V");
		PoweredOptics.Insert("crossbow_redpointoptic","Battery9V");
		PoweredOptics.Insert("reflexoptic","Battery9V");
		PoweredOptics.Insert("acogoptic","");
		PoweredOptics.Insert("puscopeoptic","");
		PoweredOptics.Insert("kashtanoptic","");
		PoweredOptics.Insert("huntingoptic","");
		PoweredOptics.Insert("pistoloptic","");
		PoweredOptics.Insert("pso1optic","");
		PoweredOptics.Insert("pso11optic","Battery9V");
		PoweredOptics.Insert("grozaoptic","");
		PoweredOptics.Insert("kobraoptic","Battery9V");

		ConstructLoadouts(false);
	}

	//=====================Export Functions====================
	array<string> ConstructCargoArray(ItemBase SentItem)
	{
		//Gets cargo within cargo as well
		array<string> ret = new array<string>;
		InventoryItem item;
		InventoryItemBase SubinventoryItem;
		Class.CastTo(item, SentItem);
	
		if (!item) return NULL;
	
		if (item.GetInventory().GetCargo())
		{
			CargoBase cargo = item.GetInventory().GetCargo();
			for (int j = 0; j < cargo.GetItemCount(); j++)
			{
				InventoryItemBase inventoryItem;
				Class.CastTo(inventoryItem, cargo.GetItem(j));
				ret.Insert(inventoryItem.GetType());

				if (inventoryItem.GetInventory().GetCargo()) //Check for Cargo within cargo
				{
					CargoBase Subcargo = inventoryItem.GetInventory().GetCargo();
					for (j = 0; j < Subcargo.GetItemCount(); ++j)
					{
						SubinventoryItem = Subcargo.GetItem(j);
						ret.Insert(SubinventoryItem.GetType());
					}
				}
				else
				{
					if (SubinventoryItem != NULL && SubinventoryItem.GetInventory().AttachmentCount())
					{
						for (j = 0; j < SubinventoryItem.GetInventory().AttachmentCount(); ++j)
						{
							InventoryItemBase SubAtt;
							Class.CastTo(SubAtt, SubinventoryItem);
							ret.Insert(SubAtt.GetInventory().GetAttachmentFromIndex(j).GetType());
						}
					}
				}
			}
		}
		else
		{
			for (j = 0; j < item.GetInventory().AttachmentCount(); j++)
			{
				Class.CastTo(inventoryItem, item.GetInventory().GetAttachmentFromIndex(j));
				ret.Insert(inventoryItem.GetType());
			}
		}

		return ret;
	}

	void ExportInventoryByClassName(ref array<string> ItemsArray)
	{
		PlayerBase m_DummyUnit;
		m_DummyUnit = PlayerBase.Cast(  GetGame().CreateObject( "SurvivorM_Mirek", "0 0 0", false) );
	    for (int i = 0; i < ItemsArray.Count(); ++i)
	    {
	    	m_DummyUnit.GetHumanInventory().CreateInInventory( ItemsArray.Get(i) );
	    }
	    ExportInventory(m_DummyUnit,"Defualt Loadout");
	}

	void ExportInventory(PlayerBase player,string PresetName)
	{
		ItemBase Hands,Shoulder,Melee,HeadGear,Mask,EyeWear,Gloves,Armband,Body,Vest,Back,Legs,Feet;

		Class.CastTo(Hands,player.GetHumanInventory().GetEntityInHands());
		Class.CastTo(Shoulder,player.GetHumanInventory().FindAttachment(InventorySlots.SHOULDER));
		Class.CastTo(Melee,player.GetHumanInventory().FindAttachment(InventorySlots.MELEE));
		Class.CastTo(HeadGear,player.GetHumanInventory().FindAttachment(InventorySlots.HEADGEAR));
		Class.CastTo(Mask,player.GetHumanInventory().FindAttachment(InventorySlots.MASK));
		Class.CastTo(EyeWear,player.GetHumanInventory().FindAttachment(InventorySlots.EYEWEAR));
		Class.CastTo(Gloves,player.GetHumanInventory().FindAttachment(InventorySlots.GLOVES));
		Class.CastTo(Armband,player.GetHumanInventory().FindAttachment(InventorySlots.ARMBAND));
		Class.CastTo(Body,player.GetHumanInventory().FindAttachment(InventorySlots.BODY));
		Class.CastTo(Vest,player.GetHumanInventory().FindAttachment(InventorySlots.VEST));
		Class.CastTo(Back,player.GetHumanInventory().FindAttachment(InventorySlots.BACK));
		Class.CastTo(Legs,player.GetHumanInventory().FindAttachment(InventorySlots.LEGS));
		Class.CastTo(Feet,player.GetHumanInventory().FindAttachment(InventorySlots.FEET));
		
		ExportToJSON(Hands,Shoulder,Melee,HeadGear,Mask,EyeWear,Gloves,Armband,Body,Vest,Back,Legs,Feet,PresetName);
	}

	void ExportToJSON(ItemBase Hands, ItemBase Shoulder, ItemBase Melee ,ItemBase HeadGear,ItemBase Mask,ItemBase EyeWear,ItemBase Gloves,ItemBase Armband,ItemBase Body,ItemBase Vest,ItemBase Back,ItemBase Legs,ItemBase Feet,string PresetName)
	{
		ref array<string> ItemCargo;

		int folderKey = Math.RandomIntInclusive(1,10000);

		if(Body)
		{
			ItemCargo 		= ConstructCargoArray(Body);
			WriteToJSON(folderKey,"Body",Body,ItemCargo,PresetName);
		}
		if (Hands)
		{
			ItemCargo 		= ConstructCargoArray(Hands);
			WriteToJSON(folderKey,"Hands",Hands,ItemCargo,PresetName);
		}
		if(Melee)
		{
			ItemCargo 		= ConstructCargoArray(Melee);
			WriteToJSON(folderKey,"Melee",Melee,ItemCargo,PresetName);
		}
		if(Shoulder)
		{
			ItemCargo 		= ConstructCargoArray(Shoulder);
			WriteToJSON(folderKey,"Shoulder",Shoulder,ItemCargo,PresetName);
		}
		if(HeadGear)
		{
			ItemCargo 		= ConstructCargoArray(HeadGear);
			WriteToJSON(folderKey,"HeadGear",HeadGear,ItemCargo,PresetName);
		}
		if(Mask)
		{
			ItemCargo 		= ConstructCargoArray(Mask);
			WriteToJSON(folderKey,"Mask",Mask,ItemCargo,PresetName);
		}
		if(EyeWear)
		{
			ItemCargo 		= ConstructCargoArray(EyeWear);
			WriteToJSON(folderKey,"EyeWear",EyeWear,ItemCargo,PresetName);
		}
		if(Gloves)
		{
			ItemCargo 		= ConstructCargoArray(Gloves);
			WriteToJSON(folderKey,"Gloves",Gloves,ItemCargo,PresetName);
		}
		if(Armband)
		{
			ItemCargo 		= ConstructCargoArray(Armband);
			WriteToJSON(folderKey,"Armband",Armband,ItemCargo,PresetName);
		}
		if(Vest)
		{
			ItemCargo 		= ConstructCargoArray(Vest);
			WriteToJSON(folderKey,"Vest",Vest,ItemCargo,PresetName);
		}
		if(Back)
		{
			ItemCargo 		= ConstructCargoArray(Back);
			WriteToJSON(folderKey,"Back",Back,ItemCargo,PresetName);
		}
		if(Legs)
		{
			ItemCargo 		= ConstructCargoArray(Legs);
			WriteToJSON(folderKey,"Legs",Legs,ItemCargo,PresetName);
		}
		if(Feet)
		{
			ItemCargo 		= ConstructCargoArray(Feet);
			WriteToJSON(folderKey,"Feet",Feet,ItemCargo,PresetName);
		}
	}

	void WriteToJSON(int folderKey, string Catagory, ItemBase BaseItem, ref array<string> ItemCargo, string PresetName)
	{
		string static_dir  = "$profile:Static_Loadouts";
		string file_dir    = "$profile:Static_Loadouts\\Loadout_"+ folderKey.ToString();
		string file_path   =  file_dir + "\\" + Catagory;
		string JsonData;
		FileHandle currentFile;

		ref array<string> attachments = new array<string>;
		ref map<string, ref array<string>> ItemInSlotChildren = new map<string, ref array<string>>;
		ref map<string,map<string, ref array<string>>> ItemInInentory = new map<string,map<string, ref array<string>>>;
		ref map<string,map<string,map<string, ref array<string>>>> ItemInSlot = new map<string,map<string,map<string, ref array<string>>>>;
		ref map<string,map<string,map<string,map<string, ref array<string>>>>> ContainerObject = new map<string,map<string,map<string,map<string, ref array<string>>>>>;
		ref map<string,map<string,map<string,map<string,map<string, ref array<string>>>>>> SlotData = new map<string,map<string,map<string,map<string,map<string, ref array<string>>>>>>;

		if (ItemCargo)
		{
			for (int i = 0; i < ItemCargo.Count(); ++i)
			{
				attachments.Insert(ItemCargo.Get(i));
			}
		}

		ItemInSlotChildren.Insert("Inventory",attachments);
		ItemInInentory.Insert(BaseItem.GetType(),ItemInSlotChildren);
		ItemInSlot.Insert("Type",ItemInInentory);
		ContainerObject.Insert(Catagory,ItemInSlot);
		SlotData.Insert("Slot",ContainerObject);
		
		if (FileExist(static_dir))
        {
        	if (FileExist(file_dir))
        	{
        		JsonFileLoader<map<string,map<string,map<string,map<string,map<string, ref array<string>>>>>>>.JsonSaveFile(file_path + ".json", SlotData);
        	}
        	else
        	{
        		MakeDirectory(file_dir);
        		JsonFileLoader<map<string,map<string,map<string,map<string,map<string, ref array<string>>>>>>>.JsonSaveFile(file_path + ".json", SlotData);
        	}
        }
        else
        {
        	MakeDirectory(static_dir);
        	if (FileExist(file_dir))
        	{
        		JsonFileLoader<map<string,map<string,map<string,map<string,map<string, ref array<string>>>>>>>.JsonSaveFile(file_path + ".json", SlotData);
        	}
        	else
        	{
        		MakeDirectory(file_dir);
        		JsonFileLoader<map<string,map<string,map<string,map<string,map<string, ref array<string>>>>>>>.JsonSaveFile(file_path + ".json", SlotData);
        	}
        }
        SavePresetName(file_dir,PresetName);
	}

	void SavePresetName(string path, string presetName)
	{
		//Make Defualt LD is none exist
		FindFileHandle FileSearch;
		FileHandle LoadOutDir;
		string LoadoutDirectoy = path + "\\PresetName.txt";

		if (!FileExist(LoadoutDirectoy))
		{
			FileHandle currentFile = OpenFile(LoadoutDirectoy, FileMode.WRITE);
			FPrintln(currentFile,presetName);
			CloseFile(currentFile);
		}
	}

	//=========================Import + Spwn Loadout Functions =================================
	TStringArray LoadParameterArray(map<string, TStringArray> itemsMap,string param_name)
	{
		if ( itemsMap.Contains(param_name) )
		{
			return itemsMap.Get(param_name);
		}

		return NULL;
	}

	int GimmeRandom(int Max)
	{
		int count = Math.RandomIntInclusive(0,Max - 1);
		return count;
	}

	void SpawnLoadout(PlayerBase player, bool status, array<string> Directorys, bool Random)
	{
		ref map<string,map<string,map<string,map<string,map<string, TStringArray>>>>> SavedData = new map<string,map<string,map<string,map<string,map<string, TStringArray>>>>>;
		
		ref TStringArray arrAttachments = new TStringArray;
		ref map<string, TStringArray> ItemInSlotChildren = new map<string, TStringArray>;
		ref map<string,map<string, TStringArray>> ItemInInentory = new map<string,map<string, TStringArray>>;
		ref map<string,map<string,map<string, TStringArray>>> ItemInSlot = new map<string,map<string,map<string, TStringArray>>>;
		ref map<string,map<string,map<string,map<string, TStringArray>>>> ContainerObject = new map<string,map<string,map<string,map<string, TStringArray>>>>;

		string MainDirectoy = "$profile:Static_Loadouts";

		if (status)
		{
			string SelectedLoadout;
			if (Random)
			{
				SelectedLoadout = Directorys.Get(GimmeRandom(Directorys.Count()));
			}
			else
			{
				SelectedLoadout = Directorys.Get(0);
			}

			TStringArray stSlots = {"Hands","Shoulder","Melee","HeadGear","Mask","EyeWear","Gloves","Armband","Body","Vest","Back","Legs","Feet"};
			string CurentCatagory;

			for (int i = 0; i < stSlots.Count(); ++i)
			{
				CurentCatagory = stSlots.Get(i);
				string jsonSavePath = MainDirectoy + "\\" + SelectedLoadout + "\\" + CurentCatagory +".json";

				if (FileExist(jsonSavePath))
				{
					JsonFileLoader<map<string,map<string,map<string,map<string,map<string, TStringArray>>>>>>.JsonLoadFile(jsonSavePath, SavedData);

					arrAttachments     = new TStringArray;
					ItemInSlotChildren = new map<string, TStringArray>;
					ItemInInentory     = new map<string,map<string, TStringArray>>;
					ItemInSlot 		   = new map<string,map<string,map<string, TStringArray>>>;
					ContainerObject    = new map<string,map<string,map<string,map<string, TStringArray>>>>;

					string mKey,strAttachments,strClassName,strType,strSlot,strFatherslot;
					int x = 0;
					//Read maps
					for (x = 0; x < SavedData.Count(); ++x)
					{
						mKey    = SavedData.GetKey(x);
						strFatherslot = mKey;
						ContainerObject = SavedData.Get(mKey);
					}
					for (x = 0; x < ContainerObject.Count(); ++x)
					{
						mKey = ContainerObject.GetKey(x);
						strSlot = mKey;
						ItemInSlot = ContainerObject.Get(mKey);
					}
					for (x = 0; x < ItemInSlot.Count(); ++x)
					{
						mKey = ItemInSlot.GetKey(x);
						strType = mKey;
						ItemInInentory = ItemInSlot.Get(mKey);
					}
					for (x = 0; x < ItemInInentory.Count(); ++x)
					{
						mKey = ItemInInentory.GetKey(x);
						strClassName = mKey;
						ItemInSlotChildren = ItemInInentory.Get(mKey);
					}
					arrAttachments = LoadParameterArray(ItemInSlotChildren,"Inventory");

					ItemBase itemBs;
					EntityAI itemEnt;
					TStringArray ExtrasArray;
					TStringArray AttachmentsArray;
					int j = 0;
					
					if (GetGame().IsKindOf( strClassName, "Rifle_Base"))
					{
						ExtrasArray = {};
						AttachmentsArray = {};
						if (arrAttachments.Count() >= 1)
						{
							for (j = 0; j < arrAttachments.Count(); ++j)
							{
								if (GetGame().IsKindOf( arrAttachments.Get(j), "Magazine_Base") && ! (GetGame().IsKindOf( arrAttachments.Get(j), "Ammunition_Base")) )
								{
									ExtrasArray.Insert( arrAttachments.Get(j) );
								}
								else
								{
									AttachmentsArray.Insert( arrAttachments.Get(j) );
								}
							}
						}
						if (CurentCatagory == "Shoulder")
						{
							SpawnGunIn( player , strClassName, false, AttachmentsArray, ExtrasArray );
						}
						else { SpawnGunIn( player , strClassName, true, AttachmentsArray, ExtrasArray ); }
					}
					else
					{
						itemEnt = player.GetInventory().CreateInInventory( strClassName );
						itemBs = ItemBase.Cast(itemEnt);
						if (arrAttachments.Count() >= 1)
						{
							for (j = 0; j < arrAttachments.Count(); ++j)
							{
								ExtrasArray = {};
								AttachmentsArray = {};

								if (GetGame().IsKindOf( arrAttachments.Get(j), "Pistol_Base"))
								{
									if (GetGame().IsKindOf( arrAttachments.Get(j), "Magazine_Base") && ! (GetGame().IsKindOf( arrAttachments.Get(j), "Ammunition_Base")) )
									{
										ExtrasArray.Insert( arrAttachments.Get(j) );
									}
									else
									{
										AttachmentsArray.Insert( arrAttachments.Get(j) );
									}
									SpawnGunIn( player , arrAttachments.Get(j), false, AttachmentsArray, ExtrasArray );
								}
								else
								{
									itemBs.GetInventory().CreateInInventory( arrAttachments.Get(j) );
								}
							}
						}
					}
				}
			}
			if (AdvancedLoadouts.Cast(m_MissionServer.GetModule(AdvancedLoadouts)).CheckTunables("SpawnArmed")) { m_MissionServer.RandomWeaponOnStartup(player); }
	    }
	    else
	    {
	    	//Do Vanilla if no other loadouts are found
	    	ItemBase TrashBase;
			EntityAI ShitBase;

			ShitBase = player.GetInventory().CreateInInventory( "Rag" );
			TrashBase = ItemBase.Cast(ShitBase);							
			TrashBase.SetQuantity(1);
			Print("StartingEquipSetup>>ERROR:: No Static Custom Loadouts were found!");
	    }
	}

	void SpawnStaticLoadout(string LoadOutName, string UID)
	{
		PlayerBase player;
		player = PlayerBase.Cast(m_MissionServer.GetPlayerByUID(UID));
		ref array<string> TmP = new array<string>;
		TmP.Insert(GetDirByName(LoadOutName));

		SpawnLoadout(player,true,TmP,false);
	}
			
	bool LoadRandomStaticLD(PlayerBase player)
	{
		array<string> LoadoutDirectorys = new array<string>;
		FindFileHandle FileSearch;
		FileHandle LoadOutDir;
		string MainDirectoy = "$profile:Static_Loadouts";

		//Search and compile loadouts
		string FolderName;
		int 	file_attr;
		int		flags;

		if (FileExist(MainDirectoy))
		{
			string m_Path = "$profile:/Static_Loadouts";
			string path_find_pattern = m_Path + "/*";

			FileSearch = FindFile(path_find_pattern, FolderName, file_attr, flags);

			bool found = true;
			while (found)
			{
				if ( file_attr & FileAttr.DIRECTORY )
				{
					LoadoutDirectorys.Insert(FolderName);
				}
				
				found = FindNextFile(FileSearch, FolderName, file_attr);
			}
			CloseFindFile(FileSearch);
		}

		//Check b4 we go n spawn gear!
		if (LoadoutDirectorys.Count() >= 1)
		{
			player.RemoveAllItems();
			SpawnLoadout(player,true,LoadoutDirectorys,false);
			return true;
		}
		else
		{
			SpawnLoadout(player,false,LoadoutDirectorys,false);
			return false;
		}
	}

	void LoadRndGenLoadout(PlayerBase player)
	{
		player.RemoveAllItems();

		if (Bags.Count() > 0) { player.GetInventory().CreateInInventory( Bags.GetRandomElement() );  }
		if (Gloves.Count() > 0) { player.GetInventory().CreateInInventory( Gloves.GetRandomElement() ); }
		if (Vests.Count() > 0) { player.GetInventory().CreateInInventory( Vests.GetRandomElement() ); }
		if (Tops.Count() > 0) { player.GetInventory().CreateInInventory( Tops.GetRandomElement() ); }
		if (Pants.Count() > 0) { player.GetInventory().CreateInInventory( Pants.GetRandomElement()); }
		if (Boots.Count() > 0) { player.GetInventory().CreateInInventory( Boots.GetRandomElement() ); }
		if (HeadGear.Count() > 0) { player.GetInventory().CreateInInventory( HeadGear.GetRandomElement() ); }

		if (AdvancedLoadouts.Cast(m_MissionServer.GetModule(AdvancedLoadouts)).CheckTunables("SpawnArmed")) { m_MissionServer.RandomWeaponOnStartup(player); }
	}

	string GetDirByName(string LoadoutName)
	{
		string path = "null";

		array<string> LoadoutDirectorys = new array<string>;
		FindFileHandle FileSearch;
		FileHandle LoadOutDir;
		string MainDirectoy = "$profile:Static_Loadouts";

		//Search and compile loadouts
		string FolderName;
		int 	file_attr;
		int		flags;

		if (FileExist(MainDirectoy))
		{
			string m_Path = "$profile:/Static_Loadouts";
			string path_find_pattern = m_Path + "/*";

			FileSearch = FindFile(path_find_pattern, FolderName, file_attr, flags);

			bool found = true;
			while (found)
			{
				if ( file_attr & FileAttr.DIRECTORY )
				{
					LoadoutDirectorys.Insert(FolderName);
					Print("GetDirByName FolderFound: "+FolderName);
				}
				
				found = FindNextFile(FileSearch, FolderName, file_attr);
			}
			CloseFindFile(FileSearch);

		 	for (int i = 0; i < LoadoutDirectorys.Count(); ++i)
		    {
				FileHandle currentFile = OpenFile("$profile:Static_Loadouts\\" + LoadoutDirectorys.Get(i) + "\\" + "PresetName.txt", FileMode.READ);
				if (currentFile != 0)
				{
					string line_content = "";
					while ( FGets(currentFile,line_content) > 0 )
					{
						if (line_content == LoadoutName)
						{
							path = LoadoutDirectorys.Get(i);
						}
					}
					CloseFile(currentFile);
				}
			}
		}
		return path;
	}

	//Only pulls basic items, does not include attachments of loadout. Items only used for client preview area.
	void CheckStaticLoadouts( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
        if (type == CallType.Server)
        {
			array<string> LoadoutDirectorys = new array<string>;
			FindFileHandle FileSearch;
			FileHandle LoadOutDir;
			string MainDirectoy = "$profile:Static_Loadouts";

			//Search and compile loadouts
			string FolderName;
			int 	file_attr;
			int		flags;

			if (FileExist(MainDirectoy))
			{
				string m_Path = "$profile:/Static_Loadouts";
				string path_find_pattern = m_Path + "/*";

				FileSearch = FindFile(path_find_pattern, FolderName, file_attr, flags);

				bool found = true;
				while (found)
				{
					if ( file_attr & FileAttr.DIRECTORY )
					{
						LoadoutDirectorys.Insert(FolderName);
						Print("Folder Found: "+FolderName);
					}
					
					found = FindNextFile(FileSearch, FolderName, file_attr);
				}
				CloseFindFile(FileSearch);
			}

			if (LoadoutDirectorys.Count() >= 1)
			{
			    ref map<string,map<string,map<string,map<string,map<string, TStringArray>>>>> SavedData = new map<string,map<string,map<string,map<string,map<string, TStringArray>>>>>;
				
				ref map<string,map<string, TStringArray>> ItemInInentory = new map<string,map<string, TStringArray>>;
			    ref map<string,map<string,map<string, TStringArray>>> ItemInSlot = new map<string,map<string,map<string, TStringArray>>>;
				ref map<string,map<string,map<string,map<string, TStringArray>>>> ContainerObject = new map<string,map<string,map<string,map<string, TStringArray>>>>;

				TStringArray stSlots = {"Hands","Shoulder","Melee","HeadGear","Mask","EyeWear","Gloves","Armband","Body","Vest","Back","Legs","Feet"};
				string CurrentSlot;
				string LoadOutName;
				string folderName;

				ref array<string> ItemsArray = new array<string>;
			    for (int i = 0; i < LoadoutDirectorys.Count(); ++i)
			    {
			    	folderName = LoadoutDirectorys.Get(i);
			    	for (int x = 0; x < stSlots.Count(); ++x)
				    {
				    	CurrentSlot = stSlots.Get(x);
				    	string jsonSavePath = MainDirectoy + "\\" + folderName + "\\" + CurrentSlot +".json";

				    	if (FileExist(jsonSavePath))
						{
							FileHandle currentFile = OpenFile("$profile:Static_Loadouts\\" + folderName + "\\" + "PresetName.txt", FileMode.READ);
							if (currentFile != 0)
							{
								FGets(currentFile,LoadOutName);
							}
							CloseFile(currentFile);

							Print("FileExist: "+jsonSavePath + " PresetName: "+LoadOutName);
						    JsonFileLoader<map<string,map<string,map<string,map<string,map<string, TStringArray>>>>>>.JsonLoadFile(jsonSavePath, SavedData);

						   	int Y = 0;
							string mKey;
							for (Y = 0; Y < SavedData.Count(); ++Y)
							{
								mKey    = SavedData.GetKey(Y);
								ContainerObject = SavedData.Get(mKey);
							}
							for (Y = 0; Y < ContainerObject.Count(); ++Y)
							{
								mKey = ContainerObject.GetKey(Y);
								ItemInSlot = ContainerObject.Get(mKey);
							}
							for (Y = 0; Y < ItemInSlot.Count(); ++Y)
							{
								mKey = ItemInSlot.GetKey(Y);
								ItemInInentory = ItemInSlot.Get(mKey);
							}
					        for (int J = 0; J < ItemInInentory.Count(); ++J)
					        {
					        	string ClassName = ItemInInentory.GetKey(J);
					        	ItemsArray.Insert(ClassName);
					        }
						}
				    }
				 Param2<string,ref array<string>> send_Params = new Param2<string,ref array<string>>(LoadOutName,ItemsArray);
    		     GetRPCManager().SendRPC( "RPC_SortStaticLoadouts", "SortStaticLoadouts", send_Params, true, sender );
    		     ItemsArray.Clear();
    		     ItemsArray = new array<string>;
			    }
			}
	  	}
	}
	//==============================Misc=================================

	void ConstructLoadouts(bool update)
	{
		FileHandle currentFile;
		string line_content = "";

		if (update && VPPSpawnPoints.Cast(m_MissionServer.GetConfigByType(VPPSpawnPoints)).loadOutsType() == "CUSTOMIZABLE_LOADOUTS") {
			Bags.Clear();
			Gloves.Clear();
			Vests.Clear();
			Tops.Clear();
			Pants.Clear();
			Boots.Clear();
			HeadGear.Clear();
			Face.Clear();
		}

		if (VPPSpawnPoints.Cast(m_MissionServer.GetConfigByType(VPPSpawnPoints)).loadOutsType() == "CUSTOMIZABLE_LOADOUTS")
		{
			//Load randomized ld items
			for ( int i = 0; i < LoadoutCatagories.Count(); ++i )
			{
				string currentCatagory = LoadoutCatagories.Get(i);
				currentFile = OpenFile(m_RandomLoadoutsPath + currentCatagory + ".txt", FileMode.READ);
				if (currentFile != 0)
				{
					line_content = "";
					while ( FGets(currentFile,line_content) > 0 )
					{
						if (line_content != "" || line_content != "empty" || line_content != "null")
						{
							switch(currentCatagory)
							{
								case "Bags":
								Bags.Insert(line_content);
								break;

								case "Gloves":
								Gloves.Insert(line_content);
								break;

								case "Vests":
								Vests.Insert(line_content);
								break;

								case "Tops":
								Tops.Insert(line_content);
								break;

								case "Pants":
								Pants.Insert(line_content);
								break;

								case "Boots":
								Boots.Insert(line_content);
								break;

								case "HeadGear":
								HeadGear.Insert(line_content);
								break;

								case "Face":
								Face.Insert(line_content);
								break;
							}
						}
					}
					CloseFile(currentFile);
			    } else { Print("Error Reading Randomized loadouts! File "+ currentCatagory +" Missing"); }
			}
		}
	}

	void SpawnGunIn(PlayerBase player, string item, bool isMainGun, array<string> attachments, array<string> Extras)
	{
		EntityAI itemAI;

		ItemBase myAttachmentAI;
		EntityAI myAttachmentIB;

		EntityAI ExtraEntity;

		int MinQuantity;
		Magazine mag;

		if (isMainGun)
		{
			itemAI = EntityAI.Cast(player.GetHumanInventory().CreateInHands( item ));

			player.SetQuickBarEntityShortcut(itemAI, 1, true); //Puts gun on hotkey 2

			if ( attachments != NULL )
			{
					
				for (int i = 0; i < attachments.Count(); ++i)
				{
					if (itemAI != NULL)
					{
						myAttachmentAI = ItemBase.Cast( itemAI.GetInventory().CreateInInventory( attachments.Get(i)) );
						if (PoweredOptics.Contains(attachments.Get(i)))
						{
							myAttachmentAI.GetInventory().CreateInInventory( "Battery9V" );
						}
					}
				}
			}
				
			if ( Extras != NULL )
			{
				for (int ii = 0; ii < Extras.Count(); ++ii)
				{
					if (GetGame().IsKindOf( Extras.Get(ii), "Magazine_Base") && ! (GetGame().IsKindOf( Extras.Get(ii), "Ammunition_Base")) )
					{
						Class.CastTo(mag,player.GetHumanInventory().CreateInInventory(Extras.Get(ii)));
						MinQuantity = 2;
						if (mag)
						{
							mag.ServerSetAmmoCount(Math.RandomIntInclusive(MinQuantity,mag.GetAmmoMax()));
							player.SetQuickBarEntityShortcut(mag, 0, true);  //Puts main weapons mag on hotkey 1
						}
					}
					else
					{
						ExtraEntity = EntityAI.Cast(player.GetInventory().CreateInInventory( Extras.Get(ii) ));
					}
				}
			}
		}
		else
		{
			//For Pistols/Secondary that spawn in inevntory
			if ( item != "" ) 
			{
				itemAI = EntityAI.Cast( player.GetHumanInventory().CreateInInventory( item ) );
				player.SetQuickBarEntityShortcut(itemAI, 3, true);  //Puts the Secondary weapon on hotkey 4
			
				if ( attachments != NULL && attachments.Count() > 0 && itemAI != NULL)
				{
					for (int iz = 0; iz < attachments.Count(); ++iz)
					{
						myAttachmentIB = EntityAI.Cast(itemAI.GetInventory().CreateAttachment( attachments.Get(iz) ));
						if (PoweredOptics.Contains(attachments.Get(iz)))
						{
							myAttachmentIB.GetInventory().CreateInInventory( "Battery9V" );
						}
					}
				}
				
				if ( Extras != NULL && Extras.Count() > 0 )
				{
					for (int ip = 0; ip < Extras.Count(); ++ip)
					{
						if (GetGame().IsKindOf( Extras.Get(ip), "Magazine_Base") && ! (GetGame().IsKindOf( Extras.Get(ip), "Ammunition_Base")) )
						{
							Class.CastTo(mag,player.GetInventory().CreateInInventory( Extras.Get(ip) ));
							player.SetQuickBarEntityShortcut(mag, 1, true);   //Puts the mag for the secondary on hotkey 2

							MinQuantity = 2;
							mag.ServerSetAmmoCount(Math.RandomIntInclusive(MinQuantity,mag.GetAmmoMax()));
						}
						else
						{
							ExtraEntity = EntityAI.Cast(player.GetInventory().CreateInInventory( Extras.Get(ip) ));
						}
					}
				}
			}
		}
	}
}