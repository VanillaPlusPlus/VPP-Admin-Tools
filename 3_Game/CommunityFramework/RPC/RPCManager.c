modded class RPCManager
{
	protected ref map<string,int> m_LoginAttempts = new map<string,int>;
	protected const int VPPAT_FRAMEWORK_RPC_ID = 10420;

	void RPCManager()
	{
		EnScript.Watch(m_LoginAttempts, 0);
	}

	override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		VOnRPC(sender, target, rpc_type, ctx);
		super.OnRPC(sender, target, rpc_type, ctx);
	}

	protected void VOnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type != VPPAT_FRAMEWORK_RPC_ID)
			return;
		
		string modName  = string.Empty;
		string funcName = string.Empty;

		string recievedFrom = "server";

		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			if (!sender)
				return; //dip out, all RPCs coming from client must have a valid sender.

			recievedFrom = sender.GetPlainId();

			Param3<string, string, string> serverMetaData;
			if (!ctx.Read(serverMetaData))
				return;

			string pw = serverMetaData.param1;
			modName   = serverMetaData.param2;
			funcName  = serverMetaData.param3;

			if (!g_Game.IsPasswordProtectionDisabled())
			{
				if (pw == string.Empty)
				{
					WriteToLogs(string.Format("Player [%1] sent an RPC to server, it contained no password key! Not supposed to happen. RPC rejected.", recievedFrom));
					return;
				}

				if (g_Game.GetAdminPasswordHash() == string.Empty)
				{
					WriteToLogs("There is no password set! The tool will not function correctly. Please follow instructions found within: profile/VPPAdminTools/Permissions/credentials.txt");
					VSendRPC("RPC_MissionGameplay", "ServerLoginError", NULL, true, sender, NULL);
					return;
				}

				if (m_LoginAttempts[recievedFrom] >= 6)
					return;

				string hash = VSHA256.ComputeString(pw);
				if (hash != g_Game.GetAdminPasswordHash())
				{
					m_LoginAttempts.Set(recievedFrom, m_LoginAttempts[recievedFrom] + 1);
					VSendRPC("RPC_MissionGameplay", "LoginAttemptFail", NULL, true, sender, NULL);
					WriteToLogs(string.Format("Player %1 attempted login, and failed! Total Attempts: %2", recievedFrom, m_LoginAttempts[recievedFrom]));
					return;
				}
			}
		}

		if (GetGame().IsClient())
		{
			Param2<string, string> clientMetaData;
			if (!ctx.Read(clientMetaData))
				return;

			modName   = clientMetaData.param1;
			funcName  = clientMetaData.param2;
		}

		map< string, ref RPCMetaWrapper > functions;
		if ( m_RPCActions.Find( modName, functions ) )
		{
			RPCMetaWrapper wrapper;
			if ( functions.Find( funcName, wrapper ) )
			{
				if ( wrapper.GetInstance() )
				{
					auto functionCallData = new Param4< CallType, ref ParamsReadContext, ref PlayerIdentity, ref Object >( CallType.Server, ctx, sender, target );
				
					if ( ( GetGame().IsServer() && GetGame().IsMultiplayer() ) || ( GetGame().IsServer() && !GetGame().IsMultiplayer() && ( wrapper.GetSPExecutionType() == SingleplayerExecutionType.Server || wrapper.GetSPExecutionType() == SingleplayerExecutionType.Both ) ) ) 
					{
						GetGame().GameScript.CallFunctionParams( wrapper.GetInstance(), funcName, NULL, functionCallData );
					}

					if ( ( GetGame().IsClient() && GetGame().IsMultiplayer() ) || ( GetGame().IsServer() && !GetGame().IsMultiplayer() && ( wrapper.GetSPExecutionType() == SingleplayerExecutionType.Client || wrapper.GetSPExecutionType() == SingleplayerExecutionType.Both ) ) ) 
					{
						//Update call type
						functionCallData.param1 = CallType.Client;
						GetGame().GameScript.CallFunctionParams( wrapper.GetInstance(), funcName, NULL, functionCallData );
					}
				}
			} else
			{
				Error( recievedFrom + " tried sending " + modName + "::<" + funcName + "> which does not seem to exist!");
			}
		} else
		{
			Error( recievedFrom + " tried sending <" + modName + ">::" + funcName + " which does not seem to exist!");
		}
	}

	void VSendRPC(string modName, string funcName, Param params = NULL, bool guaranteed = false, PlayerIdentity sendToIdentity = NULL, Object sendToTarget = NULL)
	{
		string sendTo = "server";

		if ( GetGame().IsServer() && GetGame().IsMultiplayer() )
		{
			if ( sendToIdentity == NULL )
			{
				sendTo = "everyone";
			}else 
			{
				sendTo = sendToIdentity.GetPlainId();
			}
		}

		auto sendData = new array< ref Param >;
		if (GetGame().IsClient())
		{
			string pw = string.Empty;
			EnScript.GetClassVar(GetGame().GetMission(), "m_VAdminPassword", 0, pw);
			sendData.Insert(new Param3<string, string, string>(pw, modName, funcName));
		}else{
			sendData.Insert(new Param2<string,string>(modName, funcName));
		}
		sendData.Insert( params );
		
		//In case we are in the singleplayer and the data is consumed twice for both client and server, we need to add it twice. Better than making a deep copy with more complicated rules on receiving
		if ( !GetGame().IsMultiplayer() )
		{
			if ( m_RPCActions.Contains( modName ) )
			{
				if ( m_RPCActions[ modName ].Contains( funcName ) )
				{
					RPCMetaWrapper wrapper = m_RPCActions[ modName ][ funcName ];
					
					if ( ( wrapper.GetSPExecutionType() == SingleplayerExecutionType.Both ) )
					{
						sendData.Insert( params );
					}
				}
			}
		}
		GetGame().RPC( sendToTarget, VPPAT_FRAMEWORK_RPC_ID, sendData, guaranteed, sendToIdentity );
	}

	private void WriteToLogs(string data)
	{
		int hour, minute, second;
		int year, month, day;
		
		GetHourMinuteSecondUTC(hour,minute,second);
		GetYearMonthDayUTC(year,month,day);
		string log = string.Format("%1/%2/%3 | ",year.ToString(), month.ToString(), day.ToString()) + " [VPPAdminTools] " + data;
		Print(log);
		GetGame().AdminLog(log);
	}
};