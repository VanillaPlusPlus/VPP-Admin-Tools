class AdminTools extends PluginBase
{
	private ref VPPTeleportLocations m_TPLocations;
	override void OnInit()
	{
		m_TPLocations = VPPTeleportLocations.Cast(GetPluginManager().GetConfigByType(VPPTeleportLocations));
	}
	
	void AdminTools()
	{
		//---RPC's-----
        GetRPCManager().AddRPC( "RPC_AdminTools", "GetPlayerList", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "LoadTeleportPositions", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "TeleportToPosition", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "AlterStatus", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "PlayersTeleport", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "KillPlayer", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "HealPlayer", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "SendMessage", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "KickPlayer", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "GetPlayerPosition", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "SpawnObject", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "GetPlayerPermissions", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "RPC_AdminTools", "RemoveAdminByGUID", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "EnableToggles", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "ToggleGodmode", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "RPC_AdminTools", "GetPlayerInfo", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "RPC_AdminTools", "getItemCount", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "RPC_AdminTools", "DeleteObject", this, SingeplayerExecutionType.Server );
		//-------------
	}
	
    static PlayerBase GetPlayerByUID(string UID)
    {
        array<Man> players = new array<Man>;
        GetGame().GetWorld().GetPlayerList( players );
        PlayerBase selectedPlayer;
        for (int i = 0; i < players.Count(); ++i)
        {
            string PUID = players.Get(i).GetIdentity().GetId();
            string GUID = players.Get(i).GetIdentity().GetPlainId();
            if (PUID == UID || GUID == UID)
            {
                selectedPlayer = PlayerBase.Cast(players.Get(i));
            }
        }
        return selectedPlayer;
    }

	void EnableToggles( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if( type == CallType.Server )
        {
        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
        		Param1<bool>> map_param = new Param1<bool>(true);
	   			GetRPCManager().SendRPC( "RPC_MissionGameplay", "EnableToggles", map_param, true, sender);
        	}
        }
	}
	
	void ToggleGodmode( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
        if( type == CallType.Server )
        {
            if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
            {
    			PlayerBase AdminPlayer;
            	AdminPlayer = GetPlayerByUID(sender.GetPlainId());
    			if (AdminPlayer.GodModeStatus())
    			{
    				AdminPlayer.setGodMode(false);
    				//m_MissionServer.SendMessage(false,sender,"Admin Tools: ","God mode toggled OFF!",5,2,false,true,"",0,0);
    			}else{
    				AdminPlayer.setGodMode(true);
    				//m_MissionServer.SendMessage(false,sender,"Admin Tools: ","God mode toggled ON!",5,2,false,true,"",0,0);
    			}
		    }
        }
	}

    void DeleteObject(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        if (type == CallType.Server)
        {
            if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
            {
                if (target)
                {
                    GetGame().ObjectDelete(target);
                }
            }
        }
    }

    void getItemCount(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        Param1<string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
            if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
            {
                Print("Starting full map scan for: "+data.param1);
                //There is no way in hell you are gonna be able to do a scan of 10,000km radius without chewing up 32GB of RAM.
                ref map<ref Object,vector> FoundItems = new map<ref Object,vector>;
                ref map<string,vector> ItemPositons = new map<string,vector>;
                ref array<vector> ScanPockets = {
                "2834.2156 0 5488.2876",
                "9319.2832 0 3843.0049",
                "2021.5405 0 12436.389",
                "6256.8906 0 9233.7744",
                "7514.8716 0 15166.412",
                "13267.367 0 15142.394",
                "11489.976 0 9846.2529",
                "3206.5044 0 2449.9126",
                "13831.81 0 3602.8174",
                "9.15 0 8466.6152",
                };

                foreach(vector PositionXYZ : ScanPockets)
                {
                    Print("Starting scan at pocket: "+PositionXYZ.ToString());
                    ref array<Object> objects = new array<Object>;
                    GetGame().GetObjectsAtPosition3D(PositionXYZ, 3500, objects, NULL);

                    for (int i = 0; i < objects.Count(); ++i)
                    {
                        Object obj;
                        obj = Object.Cast( objects.Get(i) );
                        if (GetGame().ObjectIsKindOf(obj,data.param1) && !FoundItems.Contains(obj))
                        {
                            FoundItems.Insert(obj,obj.GetPosition());
                            ItemPositons.Insert(data.param1 +"_"+ i.ToString(),obj.GetPosition());
                        }
                    }
                    Print("Pocket Location: "+ PositionXYZ.ToString()+" Total Number Found: "+FoundItems.Count().ToString());
                }
                ref Param1<ref map<string,vector>> m_Data = new Param1<ref map<string,vector>>(ItemPositons);
                GetRPCManager().SendRPC( "RPC_ServerSettingsUI", "sortDBData", m_Data, true, sender);
            }
        }
    }

    void GetPlayerInfo(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
    {
        Param2<ref PlayerBase, int> data;
        if ( !ctx.Read( data ) ) return;
        
        if (type == CallType.Server)
        {
            if (data.param1 != NULL && data.param1.GetIdentity() != NULL && sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
            {
                string playerName    = data.param1.GetIdentity().GetName();
                string Steam64ID     = data.param1.GetIdentity().GetPlainId();
                string GUID          = data.param1.GetIdentity().GetId();
                float currentHealth  = data.param1.GetHealth("GlobalHealth", "");
                float currentBlood   = data.param1.GetHealth("GlobalHealth", "Blood");

                ref Param6<string,string,string,float,float,int> m_Data = new Param6<string,string,string,float,float,int>(playerName,Steam64ID,GUID,currentHealth,currentBlood,data.param2);
                GetRPCManager().SendRPC( "RPC_MissionGameplay", "InitESPBox", m_Data, true, sender);
            }
        }
    }

    void RemoveAdminByGUID( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
        Param1<ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
            ref array<string> m_GUIDs = new array<string>;
            m_GUIDs.Copy(data.param1);
            if (data.param1 != NULL && sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
            {
               AdminCaches.Cast(GetPluginManager().GetConfigByType(AdminCaches)).removeCacheByGUID(m_GUIDs.Get(0));
            }
        }
    }

	void GetPlayerPermissions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		//This will always contain 1 player
		Param1<ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<string> m_GUIDs = new array<string>;
        	m_GUIDs.Copy(data.param1);
        	if (data.param1 != NULL && sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
                ref array<string> m_Permissions = AdminCaches.Cast(GetPluginManager().GetConfigByType(AdminCaches)).getPermissions(m_GUIDs.Get(0));
        		
        		Param2<string,ref array<string>> map_param = new Param2<string,ref array<string>>(m_GUIDs.Get(0),m_Permissions);
	   			GetRPCManager().SendRPC( "RPC_VPPAdminHud", "SortPlayerPermissions", map_param, true, sender);
        	}
        }
	}

	void SpawnObject( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		//ClassName, SpawnType, Crosshair Pos, ItemQuantity, Damage State, array guids
		Param6<string,string,vector,float,float,ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<string> m_Guids = new array<string>;
        	string ClassName    = data.param1;
        	string SpawnType    = data.param2;
        	vector SpawnPos     = data.param3;
        	float  ItemQuantity = data.param4;
        	float  DamageState  = data.param5;

        	EntityAI ItemEntity;
        	PlayerBase TargetPlayer;
        	ref PlayerIdentity AdminIdentity = sender;
        	PlayerBase AdminPlayer;
        	ItemBase IBItem;
        	bool AI = false;
        	bool Group = false;

        	m_Guids.Copy(data.param6);
        	if (m_Guids.Count() >= 1) { Group = true; }

        	switch(DamageState)
        	{
        		case 0:
        		DamageState = 350;
        		break;

        		case 1:
        		DamageState = 75;
        		break;

        		case 2:
        		DamageState = 50;
        		break;

        		case 3:
        		DamageState = 25;
        		break;

        		case 4:
        		DamageState = 0;
        		break;
        	}

        	if (Group && AdminIdentity != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(AdminIdentity))
        	{
        		//Multi-Player Spawn
    		    for (int i = 0; i < m_Guids.Count(); ++i)
    		    {
    		    	TargetPlayer = PlayerBase.Cast( GetPlayerByUID(m_Guids.Get(i)) );
                    //TargetPlayer.ProcessDirectDamage(DT_FIRE_ARM, NULL, "head", "Bullet_556x45", "0 0 0", 100.0);
    		    	
                    if (TargetPlayer != NULL)
    		    	{
    		    		switch(SpawnType)
		        		{
		        			case "Inventory":
		        			if ( GetGame().IsKindOf( ClassName, "DZ_LightAI" ) ) return;
							ItemEntity = EntityAI.Cast( TargetPlayer.GetInventory().CreateInInventory( ClassName ) );

		        			if ( ItemEntity == NULL ) return;

		        			ItemEntity.SetHealth( ItemEntity.GetMaxHealth() );
		        			
		        			if ( ItemEntity.IsInherited( ItemBase ) )
				            {
				                IBItem = ItemBase.Cast( ItemEntity );
				             	if (ItemQuantity == -1) { ItemQuantity = IBItem.GetQuantityMax() }
				                IBItem.SetupSpawnedItem( IBItem, DamageState, ItemQuantity );
				            }
		        			break;

		        			case "Ground":
		        			if ( GetGame().IsKindOf( ClassName, "DZ_LightAI" ) ) { AI = true; }
		        			if (SpawnPos == "0 0 0")
		        			{
								ItemEntity = EntityAI.Cast( GetGame().CreateObject( ClassName, TargetPlayer.GetPosition(), false, AI ) );
		        			}
		        			else
		        			{
		        				ItemEntity = EntityAI.Cast( GetGame().CreateObject( ClassName, SpawnPos, false, AI ) );
		        			}
		        			if ( ItemEntity == NULL ) return;
		        			ItemEntity.SetHealth( ItemEntity.GetMaxHealth() );
		        			
		        			if ( ItemEntity.IsInherited( ItemBase ) && !AI )
				            {
				                IBItem = ItemBase.Cast( ItemEntity );
				                if (ItemQuantity == -1) { ItemQuantity = IBItem.GetQuantityMax(); }
				                IBItem.SetupSpawnedItem( IBItem, DamageState, ItemQuantity );
				            }
				            ItemEntity.PlaceOnSurface();
		        			break;
		        		}
    		    	}
    		    }
        	}
        	else if (!Group && AdminIdentity != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(AdminIdentity))
        	{
        		AdminPlayer = PlayerBase.Cast( GetPlayerByUID(AdminIdentity.GetPlainId()) );
        		if (AdminPlayer != NULL)
        		{
					switch(SpawnType)
	        		{
	        			case "Inventory":
	        			if ( GetGame().IsKindOf( ClassName, "DZ_LightAI" ) ) return;
							
						ItemEntity = EntityAI.Cast( AdminPlayer.GetInventory().CreateInInventory( ClassName ) );

	        			if ( ItemEntity == NULL ) return;
	        			ItemEntity.SetHealth( ItemEntity.GetMaxHealth() );
	        			
	        			if ( ItemEntity.IsInherited( ItemBase ) )
			            {
			                IBItem = ItemBase.Cast( ItemEntity );
			                if (ItemQuantity == -1) { ItemQuantity = IBItem.GetQuantityMax() }
			                IBItem.SetupSpawnedItem( IBItem, DamageState, ItemQuantity );
			            }
	        			break;

	        			case "Ground":
	        			if ( GetGame().IsKindOf( ClassName, "DZ_LightAI" ) ) { AI = true; }
	        			if (SpawnPos == "0 0 0")
	        			{
							ItemEntity = EntityAI.Cast( GetGame().CreateObject( ClassName, AdminPlayer.GetPosition(), false, AI ) );
	        			}
	        			else
	        			{
	        				ItemEntity = EntityAI.Cast( GetGame().CreateObject( ClassName, SpawnPos, false, AI ) );
	        			}
	        			if ( ItemEntity == NULL ) return;
	        			ItemEntity.SetHealth( ItemEntity.GetMaxHealth() );
	        			
	        			if ( ItemEntity.IsInherited( ItemBase ) && !AI )
			            {
			                IBItem = ItemBase.Cast( ItemEntity );
			                if (ItemQuantity == -1) { ItemQuantity = IBItem.GetQuantityMax() }
			                IBItem.SetupSpawnedItem( IBItem, DamageState, ItemQuantity );
			            }
			            ItemEntity.PlaceOnSurface();
	        			break;
	        		}
        		}
        	}
        }
	}

    void GetPlayerList( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
        Param1<string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
            if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
            {
                ref array<Man> m_PlayersList = new array<Man>;
                GetGame().GetWorld().GetPlayerList(m_PlayersList);

                ref map<string,string> m_PInfoMap = new map<string,string>; //name,guid
                for (int i = 0; i < m_PlayersList.Count(); ++i)
                {
                    m_PInfoMap.Insert(m_PlayersList.Get(i).GetIdentity().GetName(),m_PlayersList.Get(i).GetIdentity().GetPlainId());
                }

                if (m_PInfoMap != NULL)
                {
                    Param1<ref map<string,string>> param = new Param1<ref map<string,string>>(m_PInfoMap);
                    GetRPCManager().SendRPC( "RPC_VPPAdminHud", "SortPlayerList", param, true, sender);
                }
            }
        }
    }

	void GetPlayerPosition( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<string> pGUID = new array<string>;
        	ref map<string,vector> m_PositionsMap = new map<string,vector>;
        	pGUID.Copy(data.param1);
        	PlayerBase TargetPlayer;

        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
    		    for (int i = 0; i < pGUID.Count(); ++i)
    		    {
    		    	TargetPlayer = PlayerBase.Cast( GetPlayerByUID(pGUID.Get(i)) );
			    	if (TargetPlayer != NULL)
			    	{
	    		    	vector PlayerPositon = TargetPlayer.GetPosition();
	    		    	float currentHealth  = TargetPlayer.GetHealth("GlobalHealth", "");
						float currentBlood   = TargetPlayer.GetHealth("GlobalHealth", "Blood");
						float currentshock   = TargetPlayer.GetHealth("GlobalHealth", "Shock");
	    		    	
						string Fin;
						Fin = pGUID.Get(i);
						Fin += "|" + currentHealth;
						Fin += "|" + currentBlood;
						Fin += "|" + currentshock;

	    		    	m_PositionsMap.Insert(Fin,PlayerPositon); //Fianl Line, Pos
			    	}
    		    }

    		    if (m_PositionsMap != NULL)
		    	{
		    		Param1<ref map<string,vector>> map_param = new Param1<ref map<string,vector>>(m_PositionsMap);
       	 			GetRPCManager().SendRPC( "RPC_VPPAdminHud", "SortPlayerPosition", map_param, true, sender);
		    	}
        	}
        }
	}

	void KickPlayer( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<string> pGUID = new array<string>;
        	pGUID.Copy(data.param1);
        	PlayerBase TargetPlayer;
        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
    		    for (int i = 0; i < pGUID.Count(); ++i)
    		    {
    		    	TargetPlayer = PlayerBase.Cast( GetPlayerByUID(pGUID.Get(i)) );
			    	if (TargetPlayer != NULL)
			    	{
			    		PlayerIdentity TargetIdentity;
	    		    	TargetIdentity = PlayerIdentity.Cast( TargetPlayer.GetIdentity() );

	    		    	Param1<string> map_param = new Param1<string>("Kicked By Server Admin!"); //Will Add custom message that is grabbed from the serversettings.json
	       	 			GetRPCManager().SendRPC( "RPC_VanillaPlusPlus", "KickClientHandle", map_param, true, TargetIdentity);
			    	}
    		    }
        	}
        }
	}

	void SendMessage( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param3<string,string,ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<string> pGUID = new array<string>;
        	pGUID.Copy(data.param3);
        	PlayerBase TargetPlayer;
        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
    		    for (int i = 0; i < pGUID.Count(); ++i)
    		    {
    		    	TargetPlayer = PlayerBase.Cast( GetPlayerByUID(pGUID.Get(i)) );
    		    	if (TargetPlayer != NULL)
    		    	{
    		    		//Global, Identity ,Title, Message, Duration, FadeIn Time, Force show, DoFadeIn , Imagepath, Size X, Size Y
    		    		PlayerIdentity TargetIdentity;
    		    		TargetIdentity = PlayerIdentity.Cast( TargetPlayer.GetIdentity() );
						//m_MissionServer.SendMessage(false,TargetIdentity,data.param1,data.param2,10,2,false,true,"",0,0);
    		    	}
    		    }
        	}
        }
	}

	void HealPlayer( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<string> pGUID = new array<string>;
        	pGUID.Copy(data.param1);
        	PlayerBase TargetPlayer;
        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
    		    for (int i = 0; i < pGUID.Count(); ++i)
    		    {
    		    	TargetPlayer = PlayerBase.Cast( GetPlayerByUID(pGUID.Get(i)) );
    		    	if (TargetPlayer != NULL)
    		    	{
    		    		TargetPlayer.SetHealth("GlobalHealth", "", TargetPlayer.GetMaxHealth( "", ""));
    		    		TargetPlayer.SetHealth("GlobalHealth", "Blood", TargetPlayer.GetMaxHealth( "", "Blood"));
    		    		TargetPlayer.SetHealth("GlobalHealth", "Shock", TargetPlayer.GetMaxHealth( "","Shock"));
    		    		TargetPlayer.GetStatEnergy().Set(1000);
						TargetPlayer.GetStatWater().Set(1000);
						TargetPlayer.GetStatStomachEnergy().Set(300);		
						TargetPlayer.GetStatStomachWater().Set(300);
						TargetPlayer.GetStatHeatComfort().Set(0);
						TargetPlayer.GetStatStomachVolume().Set(0);
						TargetPlayer.GetStatDiet().Set(2500);
						TargetPlayer.GetStatSpecialty().Set(1);
						TargetPlayer.SetBleedingBits(0);
    		    	}
    		    }
        	}
        }
	}

	void KillPlayer( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<string> pGUID = new array<string>;
        	pGUID.Copy(data.param1);
        	PlayerBase TargetPlayer;
        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
    		    for (int i = 0; i < pGUID.Count(); ++i)
    		    {
    		    	TargetPlayer = PlayerBase.Cast( GetPlayerByUID(pGUID.Get(i)) );
    		    	if (TargetPlayer != NULL)
    		    	{
    		    		TargetPlayer.SetHealth("GlobalHealth", "", 0);
    		    	}
    		    }
        	}
        }
	}

	void PlayersTeleport( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param2<bool,ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {	
        	PlayerBase TargetPlayer;
	    	PlayerBase AdminPlayer;
        	ref array<string> pGUID = new array<string>;
        	pGUID.Copy(data.param2);
        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
        		if (data.param1)
        		{
        			for (int i = 0; i < pGUID.Count(); ++i)
	    		    {
	    		    	TargetPlayer = GetPlayerByUID(pGUID.Get(i));
	    		    	if (TargetPlayer != NULL)
	    		    	{
	    		    		AdminPlayer = GetPlayerByUID(sender.GetPlainId());
	    		    		TargetPlayer.SetPosition(AdminPlayer.GetPosition());
	    		    	}
	    		    }
        		}
        		else
        		{
        			TargetPlayer = GetPlayerByUID(pGUID.Get(0));
    		    	if (TargetPlayer != NULL)
    		    	{
    		    		AdminPlayer = GetPlayerByUID(sender.GetPlainId());
    		    		AdminPlayer.SetPosition(TargetPlayer.GetPosition());
    		    	}
        		}
        	}
        }
	}

	void AlterStatus( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param3<float,string,ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
        		ref array<string> oData = new array<string>;
        		oData.Copy(data.param3);
    		    for (int i = 0; i < oData.Count(); ++i)
    		    {
    		    	PlayerBase AdminPlayer;
	        		AdminPlayer = GetPlayerByUID( oData.Get(i) );
	        		if (AdminPlayer != NULL)
	        		{
	        			switch(data.param2)
	        			{
	        				case "Health":
							AdminPlayer.SetHealth("GlobalHealth", "", data.param1);
							break;

							case "Blood":
							AdminPlayer.SetHealth("GlobalHealth", "Blood", data.param1);
							Print("Blood GOt from cli::"+data.param1);
							break;

							case "Shock":
							float currentshock = AdminPlayer.GetHealth("GlobalHealth", "Shock");
							float NewValue = currentshock + data.param1;
							AdminPlayer.SetHealth("GlobalHealth", "Shock", NewValue);
							break;
	        			}
	        		}
    		    }
        	}
        }
	}

	void TeleportToPosition( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param3<vector,ref array<string>,bool> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	if (sender != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
        		if (data.param1)
        		{
        			vector TPPos = data.param1;
        			if (data.param3)
        			{
        				if (TPPos[1] <= 0)
	        			{
	        				GetPlayerByUID(sender.GetPlainId()).SetPosition(g_Game.SnapToGround(TPPos));
	        			}
	        			else
	        			{
	        				GetPlayerByUID(sender.GetPlainId()).SetPosition(TPPos);
	        			}
        			}
        			
        			ref array<string> pUIDS = new array<string>;
		        	if (data.param2 != NULL)
		        	{
		        		pUIDS.Copy(data.param2);
		        		//Multi-Tp
    				    for (int i = 0; i < pUIDS.Count(); ++i)
    				    {
    				    	string oUID = pUIDS.Get(i);
    				    	if (TPPos[1] <= 0)
		        			{
		        				GetPlayerByUID(oUID).SetPosition(g_Game.SnapToGround(TPPos));
		        			}
		        			else
		        			{
		        				GetPlayerByUID(oUID).SetPosition(TPPos);
		        			}
    				    }
		        	}
        		}
        	}
        }
	}
	
	void LoadTeleportPositions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	if (sender != NULL && m_TPLocations != NULL && PermitManager.Cast(GetPluginManager().GetPluginByType(PermitManager)).VerifyIdentity(sender))
        	{
                m_TPLocations.load();
                ref array<ref VPPTeleportLocation> m_TeleportLocations = m_TPLocations.getLocationArray();
                ref map<string, vector> m_TpInfo = new map<string, vector>;
                foreach(VPPTeleportLocation location : m_TeleportLocations)
                {
                    m_TpInfo.Insert(location.getName(),location.getLocation());
                }
                
          		Param1<ref map<string, vector>> map_param = new Param1<ref map<string, vector>>(m_TpInfo);
        		GetRPCManager().SendRPC( "RPC_VPPAdminHud", "SortTeleportPositions", map_param, true, sender);
        	}
        }
	}
}
