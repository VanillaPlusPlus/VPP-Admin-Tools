class BannedPlayer
{
	string playerName;
	string Steam64Id;
	string GUID;
	string banReason;
	string issuedBy;
	ref BanDuration expirationDate;
	
	void BannedPlayer(string name, string id, string guid, BanDuration duration, string issuerInfo, string reason = "No Reason.")
	{
		playerName  = name;
		Steam64Id   = id;
		GUID  	    = guid;
		banReason   = reason;
		expirationDate = duration;
		issuedBy	   = issuerInfo;
	}
}

class BanDuration
{
	int Hour,Minute;
	int Year,Month,Day;
	bool Permanent;
	
	void BanDuration(int year, int month, int day, int hour, int minute, bool unlmited = false)
	{
		Hour	  = hour;
		Minute	  = minute;
		Year	  = year;
		Month	  = month;
		Day	 	  = day;
		Permanent = unlmited;
	}
}

class BansManager extends ConfigurablePlugin
{
	private ref array<ref BannedPlayer> m_BanList;
	
	void BansManager()
	{
		JSONPATH = "$profile:VPPAdminTools/BanList.json";
		m_BanList = new array<ref BannedPlayer>;
		GetRPCManager().AddRPC("BanManagerServer", "SendData", this);
		GetRPCManager().AddRPC("BanManagerServer", "RemoveBan", this);
		GetRPCManager().AddRPC("BanManagerServer", "RemoveBans", this);
		GetRPCManager().AddRPC("BanManagerServer", "UpdateBanDuration", this);
		GetRPCManager().AddRPC("BanManagerServer", "UpdateBanReason", this);

	}
	
	override void OnInit()
	{
		Load();
	}
	
	override void Load()
	{
		if (FileExist(JSONPATH))
		{
			GetSimpleLogger().Log("[BansManager]:: Load(): Loading ban list Json File " + JSONPATH);
			JsonFileLoader<BansManager>.JsonLoadFile(JSONPATH, this);
			return;
		}
		//Add defualt and save
		m_BanList.Insert(new BannedPlayer("penacoladas","76561198130682592","O38XSClr/j9BD5Ylv1NOSZ5C95NxFqMBr+g7PL1gRz4=",new BanDuration(0,0,0,0,0,true),"Hardcoded by DaOne","Disrespectful behaviour."));
		Save();
	}
	
	override void Save()
	{
		GetSimpleLogger().Log("[BansManager]:: Save(): Saving ban list!");
		JsonFileLoader<BansManager>.JsonSaveFile(JSONPATH, this);
	}
	
	//
	//	RPC Section
	//
	void SendData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		if(type == CallType.Server)
		{
			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuBansManager", "", false)) return;
			
			GetRPCManager().SendRPC("BanManagerClient", "HandleData", new Param1<ref array<ref BannedPlayer>>(m_BanList), true, sender);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BansManager] Requested ban list"));
		}
	}
	
	void RemoveBan(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			Param1<string> data;
			if(!ctx.Read(data)) return;
			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "BansManager:UnbanPlayer")) return;
			
			string id = data.param1;
			RemoveFromBanList(id);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BansManager] Removed ban for player ID: " + id));
		}
	}
	
	void RemoveBans(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			Param1<ref array<string>> data;
			if(!ctx.Read(data)) return;

			RemoveMultipleFromBanList(data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BansManager] Removing " + data.param1.Count() + " ban(s)"));
		}
	}
	
	void UpdateBanDuration(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<ref array<string>,ref BanDuration> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "BansManager:UpdateBanDuration")) return;
			string arrCount = data.param1.Count().ToString();
			
			foreach(string id : data.param1)
			{
				ref BannedPlayer entry = GetBannedPlayer(id);
				if (entry != null)
				{
					//Update Ban Duration
					entry.expirationDate = data.param2;
				}
			}
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),string.Format("Updated [%1] Ban Duration(s)",arrCount),NotifyTypes.NOTIFY);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BansManager] Updated Ban Duration for: " + arrCount + " player(s)"));
			Save();
		}
	}
	
	void UpdateBanReason(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<ref array<string>,string> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "BansManager:UpdateBanReason")) return;
			string arrCount = data.param1.Count().ToString();
			foreach(string id : data.param1)
			{
				ref BannedPlayer entry = GetBannedPlayer(id);
				if (entry != null)
				{
					//Update Ban Reason
					entry.banReason = data.param2;
				}
			}
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),string.Format("Updated [%1] Ban Reason(s)",arrCount),NotifyTypes.NOTIFY);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BansManager] Updated Ban Reasons for: " + arrCount + " player(s)"));
			Save();
		}
	}
	
	//
	//	End RPC Section
	//	
	/*
		Adds a player to the custom ban list. Returns true if player was banned. Kicks player once banned
		@Param0: BannedPlayer class data to hold players info
	*/
	bool AddToBanList(BannedPlayer player)
	{
		if (!IsPlayerBanned(player.Steam64Id))
		{
			GetSimpleLogger().Log(string.Format("[BansManager]:: AddToBanList: Adding [%1] [%2] [%3] Reason:[%4] Is now BANNED!",player.playerName,player.Steam64Id,player.GUID,player.banReason));
			GetPermissionManager().NotifyPlayer("", string.Format("[%1] has been banned", player.playerName), NotifyTypes.NOTIFY);
			m_BanList.Insert(player);
			Save();
			
			//Kick player after ban
			PlayerIdentity identity = GetPermissionManager().GetIdentityById(player.Steam64Id);

			if(identity != null)
			{
				GetRPCManager().SendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( player.banReason ), true, identity);
				return true;
			}
			GetSimpleLogger().Log(string.Format("[BansManager]:: AddToBanList: Player [%1] [%2] [%3] Reason:[%4] is already BANNED!",player.playerName,player.Steam64Id,player.GUID,player.banReason));

		}
		GetSimpleLogger().Log(string.Format("[BansManager]:: AddToBanList: Player [%1] [%2] [%3] Reason:[%4] is already BANNED!",player.playerName,player.Steam64Id,player.GUID,player.banReason));
		return false;
	}
	
	/*
		@Param0: Can be name or any two of the guids
	*/
	void RemoveFromBanList(string id)
	{
		if (GetBannedPlayer(id) == null)
		{
			GetSimpleLogger().Log("[BansManager]:: RemoveFromBanList(): "+id+" player not in ban list!");
			return;
		}
		
		autoptr BannedPlayer player = GetBannedPlayer(id);
		if (player != null)
		{
			m_BanList.RemoveItem(player);
			GetSimpleLogger().Log("[BansManager]:: RemoveFromBanList(): Removed "+id+" from ban list!");
			Save();
		}
	}
	
	bool RemoveMultipleFromBanList(array<string> ids)
	{
		bool success = false;
		foreach(string id : ids)
		{
			autoptr BannedPlayer player = GetBannedPlayer(id);
			if(player != null)
			{
				m_BanList.RemoveItem(player);
				GetSimpleLogger().Log("[BansManager]:: RemoveFromBanList(): Removed "+id+" from ban list!");
				success = true;
			}
		}
		Save();
		return success;
	}
	
	//Works with steam64,GUID & name
	bool IsPlayerBanned(string id)
	{
		ref BannedPlayer bannedPlayer = GetBannedPlayer(id);
		if (bannedPlayer != null)
		{
			if (!CompareTime(bannedPlayer.expirationDate))
			{
				return true;
			}
			else
			{
				//Remove ban if time expired
				RemoveFromBanList(id);
				return false;
			}
		}
		return false;
	}
	
	/*
		Compare time stamps
		returns true if ban has expired
	*/
	bool CompareTime(BanDuration TimeStamp)
	{
		autoptr BanDuration currentTimeStamp = GetCurrentTimeStamp();
		Print(string.Format("Current TimeStamp: [%1] [%2] [%3] [%4] [%5]",currentTimeStamp.Year,currentTimeStamp.Month,currentTimeStamp.Day,currentTimeStamp.Hour,currentTimeStamp.Minute));
		Print(string.Format("TimeStamp To Compare: [%1] [%2] [%3] [%4] [%5]",TimeStamp.Year,TimeStamp.Month,TimeStamp.Day,TimeStamp.Hour,TimeStamp.Minute));
		int total = 0;
		
		if (TimeStamp.Permanent)
			return false;
		
		total  = (currentTimeStamp.Year - TimeStamp.Year);
		total += (currentTimeStamp.Month - TimeStamp.Month);
		total += (currentTimeStamp.Day - TimeStamp.Day);
		total += (currentTimeStamp.Hour - TimeStamp.Hour);
		total += (currentTimeStamp.Minute - TimeStamp.Minute);
		
		return total >= 0;
	}
	

	/*
		Gets machine time and puts it into a BanDuration class to use for comparing
	*/
	BanDuration GetCurrentTimeStamp()
	{
		int hour,minute,second;
		int year,month,day;
		GetHourMinuteSecondUTC(hour,minute,second);
		GetYearMonthDayUTC(year,month,day);
		
		return new BanDuration(year,month,day,hour,minute);
	}
	
	//Works with steam64,GUID & name
	ref BannedPlayer GetBannedPlayer(string id)
	{
		foreach(BannedPlayer player : m_BanList)
		{
			if (player.Steam64Id == id || player.GUID == id || player.playerName == id)
			{
				return player;
			}
		}
		return null;
	}
}

/* Global getter */
BansManager GetBansManager()
{
	return BansManager.Cast(GetPluginManager().GetPluginByType(BansManager));
}