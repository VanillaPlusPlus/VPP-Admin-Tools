class ClientPlayerListManager : PluginBase
{
    private ref array<ref VPPUser> m_Users;

    void ClientPlayerListManager()
    {
        m_Users = new array<ref VPPUser>;

        GetRPCManager().AddRPC( "RPC_ClientPlayerList",   "HandleConnectionList", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "RPC_ClientPlayerList",   "ReceiveAddData", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "RPC_ClientPlayerList",   "ReceiveDeleteData", this, SingeplayerExecutionType.Server );
    }
	
	int GetCount()
	{
		return m_Users.Count();
	}
	
	array<ref VPPUser> GetUsers()
	{
		return m_Users;
	}

    void HandleConnectionList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<array<ref VPPUser>> data;

        if(type == CallType.Client)
        {
            if(!ctx.Read(data)) return;
			delete m_Users;
			m_Users = new array<ref VPPUser>;
            m_Users = data.param1;
        }
    }

    void ReceiveAddData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
    {
        Param1<ref VPPUser> data;

        if(type  == CallType.Client)
        {
            if(!ctx.Read(data)) return;
			
			VPPUser temp = data.param1;
			
			foreach(VPPUser user : m_Users)
			{
				if(user.GetUserId() == temp.GetUserId())
				{
					return;
				}
			}
            m_Users.Insert(data.param1);
        }
    }

    void ReceiveDeleteData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
    {
        Param1<string> data;

        if(type  == CallType.Client)
        {
            if(!ctx.Read(data)) return;

            string id = data.param1;

            foreach(VPPUser user : m_Users)
            {
                if(user.GetUserId() == id)
                {
                    m_Users.RemoveItem(user);
					return;
                }
            }
        }
    }

    void SendPlayerList(PlayerIdentity identity)
    {
        if(GetGame().IsServer() && GetGame().IsMultiplayer())
        {
            GetRPCManager().VSendRPC( "RPC_ClientPlayerList", "HandleConnectionList", new Param1<array<ref VPPUser>>(m_Users), true, identity);
        }
    }

    void AddUserServer(string name, string id, int sessionId)
    {
        if(GetGame().IsServer() && GetGame().IsMultiplayer())
        {
            VPPUser user = new VPPUser(name, id, sessionId);
            m_Users.Insert(user);
            GetRPCManager().VSendRPC( "RPC_ClientPlayerList", "ReceiveAddData", new Param1<ref VPPUser>(user), true);
        }
    }

    void RemoveUserServer(string id)
    {
        if(GetGame().IsServer() && GetGame().IsMultiplayer())
        {
            foreach(VPPUser user : m_Users)
            {
                if(user.GetUserId() == id)
                {
                    GetRPCManager().VSendRPC( "RPC_ClientPlayerList", "ReceiveDeleteData", new Param1<string>(user.GetUserId()), true);
                    m_Users.RemoveItem(user);
                    break;
                }
            }
        }
    }
	
	bool HasPlayerInList(string id)
	{
		if(GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			foreach(VPPUser user : m_Users)
			{
				if(user.GetUserId() == id)
				{
					return true;
				}
			}
		}
		
		return false;
	}
};


ClientPlayerListManager GetPlayerListManager()
{
    return ClientPlayerListManager.Cast(GetPluginManager().GetPluginByType(ClientPlayerListManager));
};