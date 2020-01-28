class AdminTools extends PluginBase
{
	void AdminTools()
	{
		//---RPC's-----
		//GetRPCManager().AddRPC( "RPC_AdminTools", "AttachTo", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "DeleteObject", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "TeleportToPosition", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "ToggleFreeCam", this, SingeplayerExecutionType.Server );
		//-------------
	}

    void DeleteObject(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "DeleteObjectAtCrosshair")) return;
            
            if (target)
            {
            	GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[AdminTools] Deleted Object @ crosshair: "  + target.GetType()));
                GetGame().ObjectDelete(target);
                GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] Just deleted an object!",sender.GetPlainId(), sender.GetName()));
            }
        }
    }
	
    void AttachTo(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
	    Param1< Object > data; //Caries player
	    if ( !ctx.Read( data ) &&  !data.param1) return;
		
	    auto relativeLocalPos = target.CoordToLocal( data.param1.GetPosition() );
	    auto playerObj = PlayerBase.Cast( data.param1 );
	    if( playerObj ){
	        vector pLocalSpaceMatrix[ 4 ];
	        pLocalSpaceMatrix[ 3 ] = relativeLocalPos;
	        playerObj.LinkToLocalSpaceOf( target, pLocalSpaceMatrix );
	        return;
	    }
	    //Attach other non player objects using AddChild
	    data.param1.SetPosition( relativeLocalPos );
	    target.AddChild( data.param1, -1, false );
	}

	void TeleportToPosition( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<vector> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportToCrosshair")) return;
			autoptr PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
			if (pb == null) return;
			
			
			if ( pb.GetCommand_Vehicle() )
			{
				Transport veh = pb.GetCommand_Vehicle().GetTransport();
				if (veh != null)
				{
					vector mat[4];
					veh.GetTransform(mat);
					mat[3] = data.param1;
					veh.SetTransform(mat);
				}
			}else{
				pb.SetPosition(data.param1);
			}
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] Teleported to crosshair position! [%3]",sender.GetPlainId(), sender.GetName(), data.param1));
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[AdminTools] Teleported to crosshair @ position: " + data.param1));
        }
	}
	
	void ToggleFreeCam(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
        if (type == CallType.Server && sender != null)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "FreeCamera")) return;
			
			GetRPCManager().SendRPC( "RPC_HandleFreeCam", "HandleFreeCam", new Param1<bool>(true), true, sender);
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] Toggled Freecam!",sender.GetPlainId(), sender.GetName()));
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[AdminTools] Toggled Freecam"));
		}
	}
}