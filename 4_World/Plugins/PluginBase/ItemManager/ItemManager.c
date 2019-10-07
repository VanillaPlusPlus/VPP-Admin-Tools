enum PlacementTypes
{
	IN_INVENTORY = 0,
	ON_GROUND    = 1,
	AT_CROSSHAIR = 2
}
enum ConditionTypes
{
	PRISTINE      = 0,
	WORN          = 1,
	DAMAGED       = 2,
	BADLY_DAMAGED = 3,
	RUINED        = 4
}

class VPPItemManager: ConfigurablePlugin
{
	private ref array<ref PresetItemData> m_SavedPresets;
	
	void VPPItemManager()
	{
		JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/ItemManager/SavedItemPresets.vpp";
		m_SavedPresets = new array<ref PresetItemData>;
		
		/*
		\ RPCs
		*/
		GetRPCManager().AddRPC("RPC_VPPItemManager", "GetData", this);
		GetRPCManager().AddRPC("RPC_VPPItemManager", "EditPreset", this);
		GetRPCManager().AddRPC("RPC_VPPItemManager", "RemoteSpawnPreset", this);
		GetRPCManager().AddRPC("RPC_VPPItemManager", "DeletePreset", this);
		GetRPCManager().AddRPC("RPC_VPPItemManager", "SpawnItem", this);
		GetRPCManager().AddRPC("RPC_VPPItemManager", "AddPreset", this);
		//--------
	}
	
	override void OnInit()
	{
		Load();
	}
	
	override void Load()
	{
		if(GetGame().IsServer() && !GetGame().IsMultiplayer()) return;
		
		MakeDirectory("$profile:VPPAdminTools/ConfigurablePlugins/ItemManager");
		
		if(FileExist(JSONPATH))
		{
			FileSerializer file = new FileSerializer();
			if (file.Open(JSONPATH, FileMode.READ))
			{
			    file.Read(m_SavedPresets);
			    file.Close();
			    GetSimpleLogger().Log("[VPPItemManager]:: Load(): Loading Cache File " + JSONPATH);
			}
		}else{
			GetSimpleLogger().Log("[VPPItemManager]:: Load(): Creating Default Cache File " + JSONPATH);
			CreateDefualtPresets();
		}
	}
	
	override void Save()
	{
		if( GetGame().IsServer() && GetGame().IsMultiplayer() )
		{
			autoptr FileSerializer file = new FileSerializer();
			if (file.Open(JSONPATH, FileMode.WRITE))
			{
			    file.Write(m_SavedPresets);
			    file.Close();
				GetSimpleLogger().Log("[VPPItemManager]:: Save() to " + JSONPATH);
			}
		}
	}
	
	private void CreateDefualtPresets()
	{
		m_SavedPresets.Insert(new PresetItemData("Civilian Sedan Car","CivilianSedan",{"civsedandoors_driver","civsedandoors_codriver","civsedandoors_backleft","civsedandoors_backright","civsedanhood","civsedantrunk","carbattery","sparkplug","civsedanwheel","civsedanwheel","civsedanwheel","civsedanwheel"}));
		m_SavedPresets.Insert(new PresetItemData("Black Camo AKM","AKM",{"ak_suppressor","ak_woodbttstck_black","ak_railhndgrd_black","pso1optic","Mag_AKM_30Rnd"}));
		Save();
	}
	
	private void SpawnItemPreset(ItemSpawnParams params, string senderID)
	{
		autoptr PresetItemData ToSpawn = GetPresetByName(params.presetName);
		if (ToSpawn == null) 
		{
			GetSimpleLogger().Log("[VPPItemManager]:: SpawnItemPreset(): Unable to spawn preset: " + params.presetName+" preset is NULL!");
			return;
		}
		
		bool hasParent;
		string parentType = ToSpawn.GetParentType();
		if (parentType == ""){
			GetSimpleLogger().Log("[VPPItemManager]:: SpawnItemPreset(): WARNING! preset: " +params.presetName+" has no parrent!");
		}
		
		//Note: Item sets cant be spawned at crosshair pos for other players.
		EntityAI parent;
		autoptr EntityAI childEntity;
		autoptr PlayerBase targetPlayer;
		autoptr array<string> items;
		if (params.targets != null && params.targets.Count() >= 1)
		{
			foreach(string targetID : params.targets)
			{
				targetPlayer = GetPermissionManager().GetPlayerBaseByID(targetID);
				if (targetPlayer == null) continue;
				
				if (params.placementType == PlacementTypes.IN_INVENTORY)
					parent = CreateEntity(parentType, "0 0 0", params.condition, params.quantity, targetPlayer);
				
				if (params.placementType == PlacementTypes.ON_GROUND || params.placementType == PlacementTypes.AT_CROSSHAIR)
					parent = CreateEntity(parentType, targetPlayer.GetPosition(), params.condition, params.quantity);
				
				items = ToSpawn.GetItemTypes();
				foreach(string itemType : items)
				{
					if (parent != null && itemType == parent.GetType() && ToSpawn.IsParent(itemType)) continue;
					
					if (parent != null)
						childEntity = CreateEntity(itemType, "0 0 0", params.condition, params.quantity, parent);
					else if (params.placementType == PlacementTypes.ON_GROUND || params.placementType == PlacementTypes.AT_CROSSHAIR)
						childEntity = CreateEntity(itemType, targetPlayer.GetPosition(), params.condition, params.quantity);
					else
						childEntity = CreateEntity(itemType, "0 0 0", params.condition, params.quantity, targetPlayer);
				}
			}
		}else{
			//Non-Targets Spawn ( admin only ) TODO: Maybe move into methods instead of copy pasta similar things
			targetPlayer = GetPermissionManager().GetPlayerBaseByID(senderID);
			if (targetPlayer == null){
				GetSimpleLogger().Log("[VPPItemManager]:: SpawnItemPreset(): FAILED TO SPAWN ITEM BY SENDER: "+senderID+" PLAYERBASE IS NULL!");
				return;
			}
			
			if (params.placementType == PlacementTypes.IN_INVENTORY)
					parent = CreateEntity(parentType, "0 0 0", params.condition, params.quantity, targetPlayer);
			
			if (params.placementType == PlacementTypes.ON_GROUND)
				parent = CreateEntity(parentType, targetPlayer.GetPosition(), params.condition, params.quantity);
			
			if (params.placementType == PlacementTypes.AT_CROSSHAIR)
				parent = CreateEntity(parentType, params.position, params.condition, params.quantity);
				
			items = ToSpawn.GetItemTypes();
			foreach(string typeName : items)
			{
				if (parent != null && typeName == parent.GetType() && ToSpawn.IsParent(typeName)) continue;
				
				if (parent != null)
					childEntity = CreateEntity(typeName, "0 0 0", params.condition, params.quantity, parent);
				else if (params.placementType == PlacementTypes.ON_GROUND)
					childEntity = CreateEntity(typeName, targetPlayer.GetPosition(), params.condition, params.quantity);
				else if (params.placementType == PlacementTypes.AT_CROSSHAIR)
					childEntity = CreateEntity(typeName, params.position, params.condition, params.quantity);
				else
					childEntity = CreateEntity(typeName, "0 0 0", params.condition, params.quantity, targetPlayer);
			}
		}
	}
	
	private EntityAI CreateEntity(string type, vector position, float health = -1, int quantity = -1, EntityAI parentEntity = null)
	{
		EntityAI itemEntity;
		ItemBase itemBase;
		bool isAi = GetGame().IsKindOf( type, "DZ_LightAI" );
		if (parentEntity != null)
			itemEntity = EntityAI.Cast( parentEntity.GetInventory().CreateInInventory(type) ); //CreateAttachment
		else
			itemEntity = EntityAI.Cast( GetGame().CreateObject( type, position, false, isAi ) );
		
		if ( itemEntity == NULL )
		return null;

		switch(health)
		{
			case ConditionTypes.PRISTINE:
			health = itemEntity.GetMaxHealth();
			break;
			
			case ConditionTypes.WORN:
			health = (itemEntity.GetMaxHealth() * 65) / 100;
			break;
			
			case ConditionTypes.DAMAGED:
			health = (itemEntity.GetMaxHealth() * 45) / 100;
			break;
			
			case ConditionTypes.BADLY_DAMAGED:
			health = (itemEntity.GetMaxHealth() * 25) / 100;
			break;
			
			case ConditionTypes.RUINED:
			health = 0;
			break;
		}
				
		if (health <= -1)
			itemEntity.SetHealth( itemEntity.GetMaxHealth() );
		else
			itemEntity.SetHealth( health );
		
		if ( itemEntity.IsInherited( ItemBase ) && !isAi )
		{
			itemBase = ItemBase.Cast( itemEntity );
			if (quantity <= -1)
				itemBase.SetupSpawnedItem( itemBase, itemEntity.GetHealth("",""), itemBase.GetQuantityMax() );
			else
				itemBase.SetupSpawnedItem( itemBase, itemEntity.GetHealth("",""), quantity );
		}
		itemEntity.PlaceOnSurface();
		itemEntity.SetSynchDirty();
		
		return itemEntity;
	}
	
	
	PresetItemData GetPresetByName(string name)
	{
		foreach(ref PresetItemData preset : m_SavedPresets)
			if (preset.GetPresetName() == name) return preset;
		return null;
	}
	
	/*
	\ RPCs Section
	*/
	void GetData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager")) return;
			Load();
			GetRPCManager().SendRPC( "RPC_MenuItemManager", "HandleData", new Param1<ref array<ref PresetItemData>>(m_SavedPresets), true, sender);
		}
	}
	
	void EditPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref PresetItemData> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager:EditPreset")) return;
			
			autoptr PresetItemData editPreset = data.param1;
			foreach(PresetItemData preset : m_SavedPresets)
			{
				if (preset.GetPresetName() == editPreset.GetPresetName())
				{
					m_SavedPresets.RemoveItem(preset);
					m_SavedPresets.Insert(editPreset);
					break;
				}
			}
			Save();
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Successfully Saved Changes to preset: "+editPreset.GetPresetName(),NotifyTypes.NOTIFY);
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] just edited item preset [%3]",sender.GetPlainId(), sender.GetName(), editPreset.GetPresetName()));
		}
	}
	
	void RemoteSpawnPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref ItemSpawnParams> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager:SpawnPreset")) return;
			
			SpawnItemPreset(data.param1,sender.GetPlainId());
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] just spawned an item preset",sender.GetPlainId(), sender.GetName()));
		}
	}
	
	void DeletePreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data; //presetName
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager:DeletePreset")) return;
			
			autoptr PresetItemData toDelete = GetPresetByName(data.param1);
			if (toDelete != null)
			{
				m_SavedPresets.RemoveItem(toDelete);
				delete toDelete;
				Save();
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Successfully deleted preset: "+data.param1,NotifyTypes.NOTIFY);
				GetRPCManager().SendRPC( "RPC_MenuItemManager", "HandleData", new Param1<ref array<ref PresetItemData>>(m_SavedPresets), true, sender);
				GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] just deleted the preset [%3]",sender.GetPlainId(), sender.GetName(), data.param1));
			}
		}
	}
	
	void SpawnItem(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref ItemSpawnParams> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager:SpawnItem")) return;
			
			autoptr ItemSpawnParams params = data.param1;
			autoptr PlayerBase targetPlayer;
			//NOTE: params.presetName in this case is the class name of the item player wishes to spawn.
			//Spawn on Targets
			if (params.targets != null && params.targets.Count() >= 1)
			{
				foreach(string targetID : params.targets)
				{
					targetPlayer = GetPermissionManager().GetPlayerBaseByID(targetID);
					if (targetPlayer == null) continue;
					
					switch(params.placementType)
					{
						case PlacementTypes.IN_INVENTORY:
						CreateEntity(params.presetName, "0 0 0", params.condition, params.quantity, targetPlayer);
						break;
						
						case PlacementTypes.AT_CROSSHAIR:
						case PlacementTypes.ON_GROUND:
						CreateEntity(params.presetName, targetPlayer.GetPosition(), params.condition, params.quantity);
						break;
					}
					GetSimpleLogger().Log("[ItemManager] :: SpawnItem(): [" + sender.GetName() + " : " + sender.GetPlainId() + "] Spawned the preset " + params.presetName + " on [" + targetPlayer.GetIdentity().GetName() + " : " + targetID + "]");
				}
			}else{
				//Self Spawn
				targetPlayer = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
				if (targetPlayer == null){
				GetSimpleLogger().Log("[VPPItemManager]:: RPC SpawnItem(): FAILED TO SPAWN ITEM BY SENDER: "+sender.GetPlainId()+" PLAYERBASE IS NULL!");
				return;
				}
					
				switch(params.placementType)
				{
					case PlacementTypes.IN_INVENTORY:
					CreateEntity(params.presetName, "0 0 0", params.condition, params.quantity, targetPlayer);
					break;
					
					case PlacementTypes.AT_CROSSHAIR:
					CreateEntity(params.presetName, params.position, params.condition, params.quantity);
					break;
				
					case PlacementTypes.ON_GROUND:
					CreateEntity(params.presetName, targetPlayer.GetPosition(), params.condition, params.quantity);
					break;
				}
			}
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] just spawned an object [%3]",sender.GetPlainId(), sender.GetName(), params.presetName));
		}
	}
	
	void AddPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager:AddPreset")) return;
			
			if (GetPresetByName(data.param1) != null){
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Error adding new preset: "+data.param1 + " already exists!",NotifyTypes.NOTIFY);
				return;
			}
			
			m_SavedPresets.Insert(new PresetItemData(data.param1,string.Empty,{}));
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Successfully added new preset: "+data.param1,NotifyTypes.NOTIFY);
			Save();
			GetRPCManager().SendRPC( "RPC_MenuItemManager", "HandleData", new Param1<ref array<ref PresetItemData>>(m_SavedPresets), true, sender);
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] Created a new item preset",sender.GetPlainId(), sender.GetName()));
		}
	}
	//----------
};