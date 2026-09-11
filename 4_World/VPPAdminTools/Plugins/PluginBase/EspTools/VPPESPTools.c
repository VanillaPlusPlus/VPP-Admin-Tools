class VPPESPTools extends PluginBase
{
	void VPPESPTools()
	{
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "DeleteItems", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "ToggleESP", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "RestPasscodeFence", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "RetriveCodeFromObj", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "ToggleMeshESP", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "BringReturnObj", this, SingeplayerExecutionType.Server );
	}
	
	void BringReturnObj(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender != null)
		{
			Param2<Object,vector> data;
			if(!ctx.Read(data))
				return;
			
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu"))
				return;

			if (!data.param1)
				return;
			
			string logMessage = "Teleport object " + data.param1.GetDisplayName() + " to position: " + data.param2;
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), logMessage));
			
			Transport veh;
			if (Class.CastTo(veh, data.param1))
			{
				vector mat[4];
				veh.GetTransform(mat);
				mat[3] = data.param2;
				veh.SetTransform(mat);
				veh.SetPosition(data.param2);
				dBodyApplyImpulse(veh, vector.Up);
			}
			else
			{
				data.param1.SetPosition(data.param2);
			}

			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Object was moved!", NotifyTypes.NOTIFY);
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) moved an object using ESP", sender.GetName(), sender.GetPlainId()));
		}
	}

	void DeleteItems(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if( type == CallType.Server && sender != null)
		{
			Param1<ref array<Object>> data;
			if( !ctx.Read( data ) ) return;
			
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu:DeleteObjects")) return;
			
			array<Object> copyArray = new array<Object>;
			copyArray.Copy(data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "Deleted Objects Using ESP tools: total:[" + copyArray.Count() +"]"));
			foreach(Object obj : copyArray)
			{
				if (obj != null)
					GetGame().ObjectDelete(obj);
			}
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Successfully Deleted ["+copyArray.Count()+"] item(s)!",NotifyTypes.NOTIFY);
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) deleted objects using ESP", sender.GetName(), sender.GetPlainId()));
		}
	}

	void ToggleESP(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if( type == CallType.Server && sender != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu", "", false)) return;

			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "just toggled ON ESP Tools."));
		}
	}

	void ToggleMeshESP(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu:PlayerMeshEsp", "", false))
				return;

			GetRPCManager().VSendRPC("RPC_HandleMeshEspToggle", "HandleMeshEspToggle", NULL, true, sender);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "just toggled ON Player Mesh ESP."));
		}
	}
	
	void RestPasscodeFence(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu:RestPasscodeFence", "", false)) return;
			
			Param1<Object> data;
			if(!ctx.Read(data))
				return;
			
			if (BasebuildingHelperFuncs.ClearComboFromLock(BaseBuildingBase.Cast(data.param1)))
			{
				Print("Passcode reset and opened: " + data.param1);
			}
		}
	}
	
	void RetriveCodeFromObj(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu:RetriveCodeFromObj", "", false)) return;
			
			Param1<EntityAI> data;
			if(!ctx.Read(data))
				return;
			
			ItemBase itemB;
			if (Class.CastTo(itemB, data.param1))
			{
				string code;
				EnScript.GetClassVar(itemB, "m_Code", 0, code);
				GetRPCManager().VSendRPC("RPC_VPPESPTools", "HandleCodeFromObj", new Param2<string,EntityAI>(code, data.param1), true, sender);
			}
		}
	}
};