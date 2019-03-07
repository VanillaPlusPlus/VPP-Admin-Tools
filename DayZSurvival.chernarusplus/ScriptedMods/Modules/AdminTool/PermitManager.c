#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\AdminTool\\Cache\\AdminCaches.c"

class PermitManager extends VPPModuleManager
{
	private ref map<string,string> m_SuperAdmins;
	private string m_AdminListPath = "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\";

	override void Init()
	{
		m_SuperAdmins            = new map<string,string>;
		FileHandle AdminUIDSFile = OpenFile(m_AdminListPath + "SuperAdmins.txt",FileMode.READ);
		ref array<string> m_ContentLines = new array<string>;
		if (AdminUIDSFile != 0)
		{
			string line_content;
			while ( FGets(AdminUIDSFile,line_content) > 0 )
			{
				m_ContentLines.Insert(line_content);
				line_content = "";
			}
			CloseFile(AdminUIDSFile);

		    for (int i = 0; i < m_ContentLines.Count(); ++i)
		    {
		    	string Curr_Line = m_ContentLines.Get(i);
		    	if (Curr_Line.Contains("##"))
		    	{
		    		//Ignore comment lines
		    	}
		    	else
		    	{
		    		TStringArray strs = new TStringArray;
					Curr_Line.Split( "|", strs );

					m_SuperAdmins.Insert(strs.Get(0),strs.Get(1)); //UID , NAME
					Print("Adding SuperAdmin: "+ strs.Get(1) + " GUID: " + strs.Get(0) + " To the SuperAdmin List!");
		    	}
		    }
		}
		else 
		{
		 Print("Error Loading Admin GUIDS! File Missing");
		}
	}

	override void onUpdate(float timeslice)
	{
		
	}

	void PermitManager( DayZSurvival m_Mission )
	{
		m_VPPConfig = new AdminCaches();
		//-----RPC's-----
		GetRPCManager().AddRPC( "RPC_PermitManager", "VerifyAdminStatus", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_PermitManager", "SaveAdminPermissions", this, SingeplayerExecutionType.Server );
		//---------------
		Print("PermitManager:: PermitManager()::");
	}

	void ~PermitManager( )
	{
		
	}
	
	bool VerifyIdentity(ref PlayerIdentity sender)
	{
		if (sender != NULL)
		{
			string GUID = sender.GetPlainId();
			if (IsAdmin(GUID))
			{
				return true;
			}
			else if (IsSuperAdmin(GUID))
			{
				return true;
			}
		}
		return false;
	}

	void SaveAdminPermissions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param3<string,string,ref array<string>> data; //Name, Guid, Perms array
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	string AdminName = data.param1;
        	string GUID      = data.param2;
        	ref array<string> m_Perms = new array<string>;
        	m_Perms.Copy(data.param3);
        	if (sender != NULL && VerifyIdentity(sender))
        	{
        		AdminCaches.Cast(m_MissionServer.GetConfigByType(AdminCaches)).removeCacheByGUID(GUID); //Check if this GUID is already listed...deletes it if found
        		ref array<ref VPPPermission> m_permissions = new array<ref VPPPermission>;
				foreach(string strPerm : m_Perms)
				{
					m_permissions.Insert(new VPPPermission(strPerm, 1));	
				}
        		AdminCaches.Cast(m_MissionServer.GetConfigByType(AdminCaches)).addCache(GUID, AdminName, m_permissions);
        	}
        }
	}

	void VerifyAdminStatus( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	if (sender != NULL)
        	{
        		Param3<ref array<string>,bool,bool> map_param;
        		string GUID = sender.GetPlainId();
        		if (IsAdmin(GUID))
        		{
        			ref array<string> m_Perms = AdminCaches.Cast(m_MissionServer.GetConfigByType(AdminCaches)).getPermissions(GUID);
        			map_param = new Param3<ref array<string>,bool,bool>(m_Perms,true,false);
        			GetRPCManager().SendRPC( "RPC_VPPAdminHud", "VerifyAdminStatus", map_param, true, sender );
        		}
        		else if (IsSuperAdmin(GUID))
        		{
        			map_param = new Param3<ref array<string>,bool,bool>(NULL,false,true);
        			GetRPCManager().SendRPC( "RPC_VPPAdminHud", "VerifyAdminStatus", map_param, true, sender );
        		}
        	}
        }
	}

	bool HasPermission( string AdminGUID, string VPPPermissionType )
	{
		ref array<string> m_Perms = AdminCaches.Cast(m_MissionServer.GetConfigByType(AdminCaches)).getPermissions(AdminGUID);
		foreach(string perm : m_Perms)
		{
			if (perm == VPPPermissionType)
			{
				return true;
			}
		}
		return false;
	}

	bool IsSuperAdmin(string SteamID)
	{
	    for (int i = 0; i < m_SuperAdmins.Count(); ++i)
	    {
	    	string GUID  = m_SuperAdmins.GetKey(i);
	    	if (SteamID == GUID)
	    	{
	    		Print("Player is SUPERADMIN!:" + SteamID);
	    		return true;
	    	}
	    }
	    return false;
	}

	bool IsAdmin(string SteamID)
	{
	    if (AdminCaches.Cast(m_MissionServer.GetConfigByType(AdminCaches)).containsCacheByGUID(SteamID)) return true;
	    return false;
	}
}
