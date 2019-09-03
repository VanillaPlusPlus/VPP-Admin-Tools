class AdminTools extends PluginBase
{
	void AdminTools()
	{
		//---RPC's-----
        //GetRPCManager().AddRPC( "RPC_AdminTools", "getItemCount", this, SingeplayerExecutionType.Server );
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
                    GetGame().ObjectDelete(target);
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

    /*void getItemCount(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
            if (sender != NULL && PermissionManager.Cast(GetPluginManager().GetPluginByType(PermissionManager)).VerifyIdentity(sender))
            {
                Print("Starting full map scan for: "+data.param1);
                //There is no way in hell you are gonna be able to do a scan of 10,000km radius without chewing up 32GB of RAM.
                autoptr map<Object,vector> FoundItems = new map<Object,vector>;
                autoptr map<string,vector> ItemPositons = new map<string,vector>;
                autoptr array<vector> ScanPockets = {
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
	*/
	void TeleportToPosition( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<vector> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportToCrosshair")) return;
			autoptr PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
			if (pb == null) return;
			
			if (pb.IsInVehicle())
			{
				autoptr Transport transportVeh = pb.GetCommand_Vehicle().GetTransport();
				if (transportVeh != null)
				{
					vector vehTransform[4];
					transportVeh.GetTransform(vehTransform);
					vehTransform[3] = data.param1;
					transportVeh.MoveInTime(vehTransform, 0.1);
				}
			}else{
				pb.SetPosition(data.param1);
			}
        }
	}
	
	void ToggleFreeCam(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
        if (type == CallType.Server && sender != null)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "FreeCamera")) return;
			
			GetRPCManager().SendRPC( "RPC_HandleFreeCam", "HandleFreeCam", new Param1<bool>(true), true, sender);
		}
	}
}