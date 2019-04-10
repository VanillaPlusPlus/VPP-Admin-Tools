modded class MissionServer
{
	private ref DEXML m_DEXML;
	void MissionServer()
	{
		//=============RPC's====================
		GetRPCManager().AddRPC( "RPC_ReadFromXML", "GetTypesXML", this, SingeplayerExecutionType.Server );
		//======================================
		
		m_DEXML = new DEXML;
	}
	
	void GetTypesXML( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		//string ItemClassName, string FieldCatagory
		Param2<string,string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
			m_DEXML.ReadFromXML(data.param1,data.param2,sender);
        }
	}
	
	override void OnEvent(EventType eventTypeId, Param params) 
	{
		super.OnEvent(eventTypeId,params);

		PlayerIdentity identity;
		switch(eventTypeId)
		{
			case ChatMessageEventTypeID:
			 ChatMessageEventParams chat_params = ChatMessageEventParams.Cast(params);
			 if (chat_params.param1 == 0 && chat_params.param2 != "") //trigger only when channel is Global == 0 and Player Name does not equal to null
				{
					Param4<int,string,string,string> request_info = new Param4<int,string,string,string>(chat_params.param1, chat_params.param2, chat_params.param3, chat_params.param4);
					ChatBasedAdminTool.Cast(GetPluginManager().GetPluginByType(ChatBasedAdminTool)).RequestHandler(request_info); //Send the param to Admintools
				}
			break;
		}
	}
}