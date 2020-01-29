//! channel, from, text, color config class
typedef Param4<int, string, string, string> AdminChatParams;

class ChatCommandManager : PluginBase
{
	private ref Timer m_TickDelay;
    private ref array<ref ChatCommand> m_ChatCommands;
	
	void ChatCommandManager()
    {
        m_ChatCommands = new array<ref ChatCommand>;
		m_TickDelay = new Timer(CALL_CATEGORY_GAMEPLAY);
    }
    
    override void OnInit()
    {
        AddChatCommand("/strip",1, "Chat:StripPlayer", true);
        AddChatCommand("/kill",1,"Chat:KillPlayer", true);
		AddChatCommand("/heal",1,"Chat:HealPlayer", true);
		AddChatCommand("/bring",1,"Chat:BringPlayer", true);
		AddChatCommand("/return",1,"Chat:ReturnPlayer", true);
		AddChatCommand("/goto", 1, "Chat:GotoPlayer", true);

		AddChatCommand("/unban", 1, "Chat:UnbanPlayer");
		AddChatCommand("/tpt",1,"Chat:TeleportToTown");
		AddChatCommand("/tpp",3,"Chat:TeleportToPoint");
		AddChatCommand("/ammo",0,"Chat:GiveAmmo");
		AddChatCommand("/spi",1, "Chat:SpawnInventory");
		AddChatCommand("/spg", 1, "Chat:SpawnOnGround");
		AddChatCommand("/sph", 1, "Chat:SpawnInHands");
        AddChatCommand("/spawncar",0,"Chat:SpawnCar");
        AddChatCommand("/refuel",0,"Chat:refuelCar");

        /*
        AddChatCommand("/freecam",1,"");
        AddChatCommand("/debug",1,"");
        */
    }
    
    /*
        @DOC: Expansion of ToolBase
        @Purpose: To allow others to create their own chat commands ontop of the ones we have already created.
        @Usage: Mod the class ChatCommandManager, override the function OnInit(), and call super.
        @Params: string command: The string of the command starting with /, or prefix of choice,
                 int argCount:Number of other arguments excluding the command string,
                 string permission: name of permission, this also add the permission automatically to the permission manager),
                 bool targetsPlayers = false: For commands that have targets other than just yourself, or no one at all.
    */
    
    void AddChatCommand(string command, int argCount, string permission, bool targetsPlayers = false)
    {
        m_ChatCommands.Insert(new ChatCommand(command, argCount, permission, targetsPlayers));
    }
    
	void ParseCommand(string commandLine, string id)
    {
        autoptr array<string> temp = new array<string>;
        autoptr array<string> args = new array<string>;

		if(!commandLine.Contains(" "))
		{
       		GetSimpleLogger().Log("[ChatCommandManager]:: ParseCommand(): Command: " + commandLine + " ID: " + id);
        	FindCommand(commandLine, id, args);
			return;
		}
		
        string command = commandLine.Substring(0,commandLine.IndexOf(" "));
		
		commandLine.Replace(command + " ", "");
		
		if(commandLine.Contains(","))
		{
			commandLine.Split(",", args);
		}
		
		if(!commandLine.Contains(","))
		{
			args.Insert(commandLine);
		}
		
        GetSimpleLogger().Log("[ChatCommandManager]:: ParseCommand(): Command: " + command + " ID: " + id + " Command Args : " + args + " Count: " + args.Count());
        FindCommand(command, id, args);
    }
	
	void FindCommand(string command, string id, array<string> args)
    {
		ref ChatCommand pickedCommand;
		
        for(int i = 0; i < m_ChatCommands.Count(); i++)
        {
			autoptr ChatCommand c = m_ChatCommands[i];
			string test = c.GetCommand();
			
            if(test == command)
            {				
				if(!GetPermissionManager().VerifyPermission(id, c.GetPermissionName(), id)) return;

                pickedCommand = c;
				break;
            }
        }

		if(pickedCommand == null)
		{
			GetSimpleLogger().Log("[Command Manager]:: ExecuteCommand(): Invalid command.");
			return;
		}
		
		ExecuteCommand(pickedCommand, id, args);
    }
	
	private array<Man> GetValidTargets(ChatCommand command, string id, array<string> args, bool getSelf = false)
    {
		ref array<Man> players = new array<Man>;
		ref Man sender;       
		ref array<Man> validTargets = new array<Man>;
		GetGame().GetPlayers(players);
		
		if(getSelf || args[0] == "self" || args == null || args.Count() == 0)
		{
			foreach(Man man : players)
			{
				if(man.GetIdentity().GetPlainId() == id)
				{
					validTargets.Insert(man);
					return validTargets;
				}
			}
		}
		
		if(args[0] == "all")
		{
			foreach(Man target : players)
			{
			    if(!target.GetIdentity()) continue;
                
                if(target.GetIdentity().GetPlainId() == id)
                {
					sender = target;
					continue;
                }
	            
                if(GetPermissionManager().VerifyPermission(id, command.GetPermissionName(), target.GetIdentity().GetPlainId()))
                {
                	validTargets.Insert(target);
                }
			}
			return validTargets;
		}
		
		foreach(Man ply : players)
       	{
            foreach(string targetName : args)
            {
				targetName.ToLower();
				
                if(!ply.GetIdentity()) continue;
                
				    string lowerCheckName = ply.GetIdentity().GetName();
					lowerCheckName.ToLower();
				
                if(ply.GetIdentity().GetPlainId() == id)
                {
					sender = target;
                }
				                
                if(lowerCheckName == targetName || (targetName == "self" && ply.GetIdentity().GetPlainId() == id))
                {
                    if(GetPermissionManager().VerifyPermission(id, command.GetPermissionName(), ply.GetIdentity().GetPlainId()))
                    {
                        validTargets.Insert(ply);
                    }
                }
           }
		}
        
        if(validTargets != null && (validTargets.Count() == 0 || validTargets.Count() != args.Count()))
        {
        	GetPermissionManager().NotifyPlayer(id, "You may not have the permission to use that command on some, or all of your targets.",NotifyTypes.NOTIFY);
        }
        
        return validTargets;
    }
	/*
        @DOC:
		@Purpose: Then logic behind the commands, override this function, and call super to extend functionality.
		@Usage: Mod the ChatCommandManager class, and override void ExecuteCommand(ChatCommand command, string id)
        @Param: ChatCommand command: Command to Execute with proper stored command args.
                string id: ID of the user who called the command.
    */
    void ExecuteCommand(ChatCommand command, string id, array<string> args)
    {
		ref Man self = GetValidTargets(command, id, args ,true)[0];
		
		if(self == null) return;
		
		string selfName;
		selfName = self.GetIdentity().GetName();

        if(command.HasPlayersAsArgs())
        {
            ref array<Man> targets = GetValidTargets(command, id, args);
			
			if(targets == null || targets.Count() == 0) return;
			
			autoptr PlayerBase pb;
			string targetName, targetId;

            foreach(Man target : targets)
            {
            	if (target == null || self == null) continue;

            	targetName = target.GetIdentity().GetName();
            	targetId   = target.GetIdentity().GetPlainId();

				switch(command.GetCommand())
                {
                    case "/strip":
						pb = PlayerBase.Cast(target);
                        pb.RemoveAllItems();
                        GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /strip command used on: " + targetName + " ID: " + targetId));
                    break;
                    
                    case "/kill":
                    	target.SetHealth(0);
                    	GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /kill command used on: " + targetName + " ID: " + targetId));                
                    break;
					                    
                    case "/heal":
						pb = PlayerBase.Cast(target);
                		pb.SetHealth( pb.GetMaxHealth( "", "" ) );
						pb.SetHealth( "","Blood", pb.GetMaxHealth( "", "Blood" ) );
						pb.GetStatHeatComfort().Set(pb.GetStatHeatComfort().GetMax());
						pb.GetStatTremor().Set(pb.GetStatTremor().GetMin());
						pb.GetStatWet().Set(pb.GetStatWet().GetMin());
						pb.GetStatEnergy().Set(pb.GetStatEnergy().GetMax());
						pb.GetStatWater().Set(pb.GetStatWater().GetMax());
						pb.GetStatDiet().Set(pb.GetStatDiet().GetMax());
						pb.GetStatSpecialty().Set(pb.GetStatSpecialty().GetMax());
						pb.SetBleedingBits(0);
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /heal command used on: " + targetName + " ID: " + targetId));
                    break;
                    
                    case "/bring":
                		GetTeleportManager().BringPlayer(target, self.GetPosition(), id);
                		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /bring command used on: " + targetName + " ID: " + targetId));
                    break;
					
					case "/return":
						GetTeleportManager().ReturnPlayer(target, id);
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /return command used on: " + targetName + " ID: " + targetId));
					break;
					
					case "/goto":
						GetTeleportManager().GotoPlayer(target, self, id);
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /goto command used on: " + targetName + " ID: " + targetId));
					break;
					
					case "/ban":
					PlayerIdentity pid = target.GetIdentity();
					string banAuthorDetails = string.Format("%1|%2",self.GetIdentity().GetName(),self.GetIdentity().GetPlainId());
					GetBansManager().AddToBanList(new BannedPlayer(targetName, targetId, pid.GetId(), new BanDuration(1990,12,12,12,12), banAuthorDetails, "Banned By VPP Admin Tools"));
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /ban command used on: " + targetName + " ID: " + targetId));
					break;
                }
            }
        }
        
        if(!command.HasPlayersAsArgs())
        {
            //Do things that are player targetable. IE. /spi
			if(!GetPermissionManager().VerifyPermission(id, command.GetPermissionName(), id)) return;			
			if(self == null) return;
			
			if(command.GetMinArgumentCount() == 0 && args.Count() == 0)
			{
				switch(command.GetCommand())
				{
					case "/spawncar":
						autoptr array<string> parts = {
							"civsedanhood", "civsedanhood", "CivSedanDoors_CoDriver", "CivSedanDoors_Driver", "CivSedanDoors_BackLeft",
							"CivSedanDoors_BackRight", "civsedantrunk","civsedanwheel","civsedanwheel",
							"civsedanwheel","civsedanwheel","SparkPlug", "EngineBelt", "CarBattery", "carradiator",
						};
					
						Car sedan;
                        sedan = Car.Cast(GetGame().CreateObject( "civiliansedan", PlayerBase.Cast(self).GetHeadingPosition(3), false, true, true ));
										
						foreach(string part : parts)
						{
							sedan.GetInventory().CreateAttachment(part);
						}
					
						for(int i = 0; i < 4; i++)
						{
							sedan.Fill(i, sedan.GetFluidCapacity(i));
						}

						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /spawncar used on self."));
	                break;
					
					case "/ammo":
						EntityAI weapon = self.GetHumanInventory().GetEntityInHands();
						if( weapon )
						{
							weapon.SetHealth( weapon.GetMaxHealth( "", "" ) );
						
							Magazine foundMag = Magazine.Cast(weapon.GetAttachmentByConfigTypeName( "DefaultMagazine" ));
							if( foundMag && foundMag.IsMagazine())
							{
								foundMag.ServerSetAmmoMax();
							}else{							  
								foundMag = Magazine.Cast(self.GetHumanInventory().CreateInInventory(g_Game.GetDefaultMagazine(weapon)));
								foundMag.ServerSetAmmoMax();
							}
							
						    m_TickDelay.Run(2.0,this,"InvokeReloadOnClient",new Param1<ref PlayerIdentity>(self.GetIdentity()));
						
							Object suppressor = ( Object ) weapon.GetAttachmentByConfigTypeName( "SuppressorBase" );
							if( suppressor )
							{
								suppressor.SetHealth( suppressor.GetMaxHealth( "", "" ) );
							}
							GetGame().RPCSingleParam(self, ERPCs.RPC_USER_ACTION_MESSAGE, new Param1<string>( "Weapon " + weapon.ConfigGetString("displayName") + " Reloaded and Repaired" ), true, self.GetIdentity());
							GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /ammo used on self."));
						}
                    break;
					
					case "/refuel": 
						Human human = Human.Cast(self);
						Car toBeFilled = Car.Cast(human.GetCommand_Vehicle().GetTransport());
					
						if(toBeFilled == null) return;
					
						EntityAI carEntity = toBeFilled;
						carEntity.SetHealth("", "",carEntity.GetMaxHealth());
						carEntity.SetHealth("Engine", "Health",carEntity.GetMaxHealth());
						carEntity.SetHealth("EngineBelt", "Health",carEntity.GetMaxHealth());
					
						ref array<EntityAI> vehParts = new array<EntityAI>;
						TStringArray SlotNames = new TStringArray;
						string cfg_path = CFG_VEHICLESPATH + " " + toBeFilled.GetType() + " attachments";
						GetGame().ConfigGetTextArray(cfg_path, SlotNames);	
						
						foreach(string carSlot : SlotNames)
								vehParts.Insert(toBeFilled.FindAttachmentBySlotName(carSlot));
					
						if (vehParts != null){
							foreach(EntityAI att : vehParts)
							{
								if (att != null)
								{
									att.SetHealth("", "Health",att.GetMaxHealth());
									att.SetSynchDirty();
								}
							}
						}
					
						float fuelReq = toBeFilled.GetFluidCapacity( CarFluid.FUEL ) - (toBeFilled.GetFluidCapacity( CarFluid.FUEL ) * toBeFilled.GetFluidFraction( CarFluid.FUEL ));
						float oilReq = toBeFilled.GetFluidCapacity( CarFluid.OIL ) - (toBeFilled.GetFluidCapacity( CarFluid.OIL ) * toBeFilled.GetFluidFraction( CarFluid.OIL ));
						float coolantReq = toBeFilled.GetFluidCapacity( CarFluid.COOLANT ) - (toBeFilled.GetFluidCapacity( CarFluid.COOLANT ) * toBeFilled.GetFluidFraction( CarFluid.COOLANT ));
						float brakeReq = toBeFilled.GetFluidCapacity( CarFluid.BRAKE ) - (toBeFilled.GetFluidCapacity( CarFluid.BRAKE ) * toBeFilled.GetFluidFraction( CarFluid.BRAKE ));
						toBeFilled.Fill( CarFluid.FUEL, fuelReq );
						toBeFilled.Fill( CarFluid.OIL, oilReq );
						toBeFilled.Fill( CarFluid.COOLANT, coolantReq );
						toBeFilled.Fill( CarFluid.BRAKE, brakeReq );
						toBeFilled.SetSynchDirty();
						GetPermissionManager().NotifyPlayer(id, carEntity.Type().ToString() + ": reparied & refueled [" +fuelReq+ "L] added, all fluids maxed",NotifyTypes.NOTIFY);
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /refuel used on self."));
					break;
				}
				return;
			}
			
			foreach(string arg : args)
			{
				if(args.Count() < command.GetMinArgumentCount()) return;
				
				switch(command.GetCommand())
            	{
	                case "/spi":
						self.GetHumanInventory().CreateInInventory(arg);
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /spi used on self. Input:  " + arg));
	                break;
	                					
	                case "/spg":
						EntityAI itemEntity;
						ItemBase itemBase;
						bool isAi = GetGame().IsKindOf( arg, "DZ_LightAI" );
	        			itemEntity = EntityAI.Cast( GetGame().CreateObject( arg, PlayerBase.Cast(self).GetHeadingPosition(1), false, isAi ) );
				
	        			if ( itemEntity == NULL )
							return;
					
	        			itemEntity.SetHealth( itemEntity.GetMaxHealth() );
	        			
	        			if ( itemEntity.IsInherited( ItemBase ) && !isAi )
			            {
			                itemBase = ItemBase.Cast( itemEntity );
			                itemBase.SetupSpawnedItem( itemBase, itemBase.GetMaxHealth("",""), itemBase.GetQuantityMax() );
			            }
			            itemEntity.PlaceOnSurface();
			            GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /spg used on self. Input:  " + arg));
	                break;
	                
	                case "/sph":
	                	GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /sph used on self. Input:  " + arg));
						if(self.GetHumanInventory().GetEntityInHands() == null)
							self.GetHumanInventory().CreateInHands(arg);
						else
							self.GetInventory().CreateInInventory(arg);
	                break;
	                
	                case "/tpt":
	                	GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /tpt used on self. Input:  " + arg));
						GetTeleportManager().TeleportToTown(arg, self);
	                break;
					
					
					case "/tpp":
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /tpp used on self. Input:  " + args));
						GetTeleportManager().TeleportToPoint(args, self, id);
					return;
					
					case "/unban":
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, selfName, "Chat Command Manager: /unban used. Input:  " + args));
						if (GetBansManager().RemoveMultipleFromBanList(args))
							GetPermissionManager().NotifyPlayer(id,"Successfully Removed ("+args.Count()+") bans from ban list!",NotifyTypes.NOTIFY);
							else
							GetPermissionManager().NotifyPlayer(id,"User(s) information is not in ban list!",NotifyTypes.ERROR);
					return;
	            }			
			}
        }
	}
	
	/*Misc*/
	void InvokeReloadOnClient(PlayerIdentity clientIdentity)
	{
		GetRPCManager().SendRPC( "RPC_PlayerBase", "InvokeReload", null, true, clientIdentity);
	}
}

ChatCommandManager GetChatCommandManager()
{
    return ChatCommandManager.Cast(GetPluginManager().GetPluginByType(ChatCommandManager));
}