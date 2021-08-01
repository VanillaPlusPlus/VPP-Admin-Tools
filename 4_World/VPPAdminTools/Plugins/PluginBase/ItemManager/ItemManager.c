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
			}
		}else{
			CreateDefualtPresets();
		}
	}
	
	override void Save()
	{
		if( GetGame().IsServer() && GetGame().IsMultiplayer() )
		{
			FileSerializer file = new FileSerializer();
			if (file.Open(JSONPATH, FileMode.WRITE))
			{
			    file.Write(m_SavedPresets);
			    file.Close();
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
		PresetItemData ToSpawn = GetPresetByName(params.presetName);
		if (ToSpawn == null) 
		{
			GetSimpleLogger().Log(string.Format("(steamid=%1) Spawned preset: (%2)", senderID, params.presetName));
			return;
		}
		
		bool hasParent;
		string parentType = ToSpawn.GetParentType();
		if (parentType == "")
		{
			GetSimpleLogger().Log("[VPPItemManager]:: SpawnItemPreset(): WARNING! preset: " +params.presetName+" has no parrent!");
		}
		
		//Note: Item sets cant be spawned at crosshair pos for other players.
		EntityAI parent;
		EntityAI childEntity;
		PlayerBase targetPlayer;
		array<string> items;
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
			if (targetPlayer == null)
			{
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
	
	private EntityAI CreateEntity(string type, vector position, float health = -1, int quantity = -1, EntityAI parentEntity = null, bool debugSpawn = false)
	{
		EntityAI itemEntity;
		ItemBase itemBase;
		bool isAi = GetGame().IsKindOf( type, "DZ_LightAI" ) || GetGame().IsKindOf( type, "SurvivorBase" );
		int iFlags = ECE_SETUP|ECE_KEEPHEIGHT|ECE_PLACE_ON_SURFACE;
		
		if ( isAi )
			iFlags = (ECE_CREATEPHYSICS * 2)|ECE_CREATEPHYSICS|iFlags;

		if (parentEntity != null)
			itemEntity = EntityAI.Cast( parentEntity.GetInventory().CreateInInventory(type) ); //CreateAttachment
		else
			itemEntity = EntityAI.Cast( GetGame().CreateObjectEx( type, position, iFlags) );
		
		if ( itemEntity == NULL )
			return null;
		
		if (Transport.Cast(itemEntity))
		{
			dBodyApplyImpulse(itemEntity, vector.Up);
			if (debugSpawn)
				itemEntity.OnDebugSpawn();
		}

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
			{
				int varQuantityInit = GetGame().ConfigGetInt("cfgVehicles " + itemBase.GetType() + " varQuantityInit");
				if ( varQuantityInit <= -1 )
					varQuantityInit = itemBase.GetQuantityMax();
				
				itemBase.SetupSpawnedItem( itemBase, itemEntity.GetHealth("",""), varQuantityInit );
			}else{
				itemBase.SetupSpawnedItem( itemBase, itemEntity.GetHealth("",""), quantity );
			}
		}
		itemEntity.PlaceOnSurface();
		itemEntity.SetLifetime(itemEntity.GetLifetimeMax());
		itemEntity.SetSynchDirty();
		
		return itemEntity;
	}
	
	
	PresetItemData GetPresetByName(string name)
	{
		foreach(PresetItemData preset : m_SavedPresets)
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
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager", "", false)) return;
			Load();
			GetRPCManager().SendRPC( "RPC_MenuItemManager", "HandleData", new Param1<ref array<ref PresetItemData>>(m_SavedPresets), true, sender);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ItemManager] Sent saved presets data."));
		}
	}
	
	void EditPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref PresetItemData> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager:EditPreset")) return;
			
			PresetItemData editPreset = data.param1;
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
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) edited preset: (%3)", sender.GetName(), sender.GetPlainId(), editPreset.GetPresetName()));
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ItemManager] Edited preset: " + editPreset.GetPresetName()));
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
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) spawned preset: (%3)", sender.GetName(), sender.GetPlainId(), data.param1.presetName));
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ItemManager] Spawn preset: " + data.param1.presetName));
		}
	}
	
	void DeletePreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data; //presetName
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuItemManager:DeletePreset")) return;
			
			PresetItemData toDelete = GetPresetByName(data.param1);
			if (toDelete != null)
			{
				m_SavedPresets.RemoveItem(toDelete);
				delete toDelete;
				Save();
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Successfully deleted preset: "+data.param1,NotifyTypes.NOTIFY);
				GetRPCManager().SendRPC( "RPC_MenuItemManager", "HandleData", new Param1<ref array<ref PresetItemData>>(m_SavedPresets), true, sender);
				GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) deleted the preset: (%3)", sender.GetName(), sender.GetPlainId(), data.param1));

				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ItemManager] Delete preset: " + data.param1));
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
			PlayerBase targetPlayer;
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
						CreateEntity(params.presetName, "0 0 0", params.condition, params.quantity, targetPlayer, true);
						break;
						
						case PlacementTypes.AT_CROSSHAIR:
						case PlacementTypes.ON_GROUND:
						CreateEntity(params.presetName, targetPlayer.GetPosition(), params.condition, params.quantity, NULL, true);
						break;
					}
					GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) Spawned Item: (%3) on \"%4\" (steamid=%5)", sender.GetName(), sender.GetPlainId(), params.presetName, targetPlayer.VPlayerGetName(), targetID));
				}
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ItemManager] :: SpawnItem(): Spawned Item " + params.presetName + " " + params.targets.Count() + " player(s) "));
			}else{
				//Self Spawn
				targetPlayer = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
				if (targetPlayer == null){
				GetSimpleLogger().Log("[VPPItemManager]:: RPC SpawnItem(): FAILED TO SPAWN ITEM BY SENDER: "+sender.GetPlainId()+" PLAYERBASE IS NULL!");
				return;
				}

				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ItemManager] spawn object on self. item: " + params.presetName));
					
				switch(params.placementType)
				{
					case PlacementTypes.IN_INVENTORY:
					CreateEntity(params.presetName, "0 0 0", params.condition, params.quantity, targetPlayer, true);
					break;
					
					case PlacementTypes.AT_CROSSHAIR:
					CreateEntity(params.presetName, params.position, params.condition, params.quantity, NULL, true);
					break;
				
					case PlacementTypes.ON_GROUND:
					CreateEntity(params.presetName, targetPlayer.GetPosition(), params.condition, params.quantity, NULL, true);
					break;
				}
			}
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) Spawned Item: (%3)", sender.GetName(), sender.GetPlainId(), params.presetName));
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
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) Created a new item preset: (%3)", sender.GetName(), sender.GetPlainId(), data.param1));

			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), " Created a new item preset: " + data.param1 ));
		}
	}
	//----------
};