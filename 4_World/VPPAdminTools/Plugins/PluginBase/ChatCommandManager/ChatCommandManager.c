//! channel, from, text, color config class
typedef Param4<int, string, string, string> AdminChatParams;

class ChatCommandManager : PluginBase
{
    private ref array<ref ChatCommand> m_ChatCommands;
	
	void ChatCommandManager()
    {
        m_ChatCommands = new array<ref ChatCommand>;
    }
    
    override void OnInit()
    {
        AddChatCommand(new StripPlayerChatModule("/strip", 1, "Chat:StripPlayer", true));
        AddChatCommand(new KillPlayerChatModule("/kill",1,"Chat:KillPlayer", true));
		AddChatCommand(new HealPlayerChatModule("/heal",1,"Chat:HealPlayer", true));
		AddChatCommand(new BringPlayerChatModule("/bring",1,"Chat:BringPlayer", true));
		AddChatCommand(new ReturnPlayerChatModule("/return",1,"Chat:ReturnPlayer", true));
		AddChatCommand(new GotoPlayerChatModule("/goto", 1, "Chat:GotoPlayer", true));
		
		if (VPPAT_IsModLoadded("Namalsk_Survival"))
		{
			AddChatCommand(new NamalskEvents("/runevent", 1, "Chat:NamalskRunEvent"));
		}
		AddChatCommand(new RemoveBanChatModule("/unban", 1, "Chat:UnbanPlayer"));
		AddChatCommand(new TeleportToTownChatModule("/tpt",1,"Chat:TeleportToTown"));
		AddChatCommand(new TeleportToPointChatModule("/tpp",3,"Chat:TeleportToPoint"));
		AddChatCommand(new SpawnAmmoChatModule("/ammo",0,"Chat:GiveAmmo"));
		AddChatCommand(new SpawnInsideInventoryChatModule("/spi",1, "Chat:SpawnInventory"));
		AddChatCommand(new SpawnOnGroundChatModule("/spg", 1, "Chat:SpawnOnGround"));
		AddChatCommand(new SpawnInHandsChatModule("/sph", 1, "Chat:SpawnInHands"));
        AddChatCommand(new SpawnCarChatModule("/spawncar",0,"Chat:SpawnCar"));
        AddChatCommand(new RefuelVehicleChatModule("/refuel",0,"Chat:refuelCar"));
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
    
    void AddChatCommand(ChatCommand command)
    {
        m_ChatCommands.Insert(command);
    }
    
	void ParseCommand(string commandLine, string id)
    {
        array<string> temp = new array<string>;
        array<string> args = new array<string>;

		if(!commandLine.Contains(" "))
		{
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
		
       FindCommand(command, id, args);
    }
	
	void FindCommand(string command, string id, array<string> args)
    {
		ChatCommand pickedCommand;
		
        for(int i = 0; i < m_ChatCommands.Count(); i++)
        {
			ChatCommand c = m_ChatCommands[i];
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
			return;
		}
		
		ExecuteCommand(pickedCommand, id, args);
    }
	
	private array<Man> GetValidTargets(ChatCommand command, string id, array<string> args, bool getSelf = false)
    {
		array<Man> players = new array<Man>;
		Man sender;       
		array<Man> validTargets = new array<Man>;
		GetGame().GetPlayers(players);
		
		if(getSelf || args[0] == "self" || args == null || args.Count() == 0)
		{
			foreach(Man man : players)
			{
				if(PlayerBase.Cast(man).VPlayerGetSteamId() == id)
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
			    if(!target) continue;
                
                if(PlayerBase.Cast(target).VPlayerGetSteamId() == id)
                {
					sender = target;
					continue;
                }
	            
                if(GetPermissionManager().VerifyPermission(id, command.GetPermissionName(), PlayerBase.Cast(target).VPlayerGetSteamId()))
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
				
                if(!ply) continue;
                
			    string lowerCheckName = PlayerBase.Cast(ply).VPlayerGetName();
				lowerCheckName.ToLower();
				
                if(PlayerBase.Cast(ply).VPlayerGetSteamId() == id)
                {
					sender = target;
                }
				                
                if(lowerCheckName == targetName || (targetName == "self" && ply.GetIdentity().GetPlainId() == id))
                {
                    if(GetPermissionManager().VerifyPermission(id, command.GetPermissionName(), PlayerBase.Cast(ply).VPlayerGetSteamId()))
                    {
                        validTargets.Insert(ply);
                    }
                }
           }
		}
        
        if(validTargets != null && (validTargets.Count() == 0 || validTargets.Count() != args.Count()))
        {
        	GetPermissionManager().NotifyPlayer(id, "#VSTR_MISSING_PERM",NotifyTypes.NOTIFY);
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
		PlayerBase self = PlayerBase.Cast(GetValidTargets(command, id, args ,true)[0]);
		array<Man> players = new array<Man>;

		if(command.HasPlayersAsArgs())
			players = GetValidTargets(command, id, args);

		if(self == null) return;
		if(players.Count() == 0 && command.HasPlayersAsArgs() || players == null) return;
		if(args.Count() < command.GetMinArgumentCount()) return;

		command.ExecuteCommand(self, players, args);
	}
}

ChatCommandManager GetChatCommandManager()
{
    return ChatCommandManager.Cast(GetPluginManager().GetPluginByType(ChatCommandManager));
}