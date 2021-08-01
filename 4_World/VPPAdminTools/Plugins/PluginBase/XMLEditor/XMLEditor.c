class XMLEditor extends PluginBase
{
	private ref array<string> ELEMENTS = { "nominal", "lifetime", "restock", "min", "quantmin", "quantmax", "cost" };
	private string XML_PATH 		= "$profile:VPPAdminTools/Exports/types.xml";
	private string MISSION_XML_PATH = GetMissionPath("\\db\\types.xml");
	
	void XMLEditor()
	{
		/* RPCs */
		GetRPCManager().AddRPC( "RPC_XMLEditor", "GetDetails", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_XMLEditor", "EditElemets", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_XMLEditor", "GetScanInfo", this, SingeplayerExecutionType.Server );
				
		if (!FileExist(XML_PATH))
		{
			//move Types.xml from mission to profile
			PluginFileHandler fileHandle = PluginFileHandler.Cast(GetPluginManager().GetPluginByType(PluginFileHandler));
			if (fileHandle)
				fileHandle.FileDuplicate(MISSION_XML_PATH, XML_PATH);
		}
	}
	
	string GetMissionPath(string endfilePath)
	{
		string result;
		string cfgPath;
		GetGame().CommandlineGetParam("config", cfgPath);
		string fullPathToConfig = "$CurrentDir:" + cfgPath;

		if ( cfgPath == string.Empty )
			fullPathToConfig = "$CurrentDir:serverDZ.cfg";

		FileHandle serverCfg = OpenFile(fullPathToConfig, FileMode.READ);
		if (serverCfg != 0)
		{
			array<string> cfgData = new array<string>;
			string line_content = "";
			int char_count = FGets( serverCfg,  line_content );
			cfgData.Insert(line_content);
			
			while ( char_count != -1 )
			{
				cfgData.Insert(line_content);
				char_count = FGets( serverCfg,  line_content );
			}
			CloseFile(serverCfg);
			
			//Parse and find used mission
			foreach(string line: cfgData)
			{
				while(line.Contains(" "))
				{
					line.Replace(" ","");
				}
				
				if (line.Length() >= 10)
				{
					string tmpLine = line.Substring(0,10);
					if (tmpLine == "template=\"")
					{
						int strIndex;
						while(line[strIndex] != string.Empty)
						{
							if (line[strIndex] == "\"" && line[strIndex + 1] != string.Empty)
							{
								if (line[strIndex + 1] == ";")
								{
									string dirtyName = line.Substring(0,strIndex);
									dirtyName.Replace("template=\"", "");
									result = "$CurrentDir:mpmissions\\" + dirtyName + endfilePath;
									break;
								}
							}
							strIndex++;
						}
					}
				}
			}
		}
		return result;
	}
	
	void GetDetails(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
	 	if ( type == CallType.Server )
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuXMLEditor", "", false)) return;
			
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[XMLEditor] Requested xml data for item: " + data.param1));

			XMLParser parser = new XMLParser(XML_PATH);
			if (parser.Load() && parser.VerifyXml())
			{
				array<ref Param3<string,string,int>> result = new array<ref Param3<string,string,int>>;
				
				foreach(string elementType : ELEMENTS)
				{
					array<ref LineElement> Xmldata = parser.GetElementString("type",data.param1);
					LineElement elementData        = parser.GetElementStringValue(elementType,Xmldata);
					if (elementData == null || elementData.data == "" || elementData.index <= -1)
						result.Insert(new Param3<string,string,int>(elementType,"#VSTR_NOT_DEFINED",-1));
					else
						result.Insert(new Param3<string,string,int>(elementType,elementData.data,elementData.index));
				}
				if (sender != null)
					GetRPCManager().SendRPC("RPC_MenuXMLEditor", "HandleDetails", new Param2<string, ref array<ref Param3<string,string,int>>>(data.param1,result), true, sender);
			}
		}
	}
	
	void EditElemets(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string, ref array<ref Param3<string,string,int>>> data;
		if (!ctx.Read(data)) return;
		
	 	if ( type == CallType.Server )
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuXMLEditor", "", false)) return;
			
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[XMLEditor] Saved edits to type.xml"));

			array<ref Param3<string,string,int>> paramsData = data.param2;
			string  itemType = data.param1;
			
			XMLParser parser = new XMLParser(XML_PATH);
			if (parser.Load() && parser.VerifyXml())
			{
				foreach(Param3<string,string,int>> params: paramsData)
				{
					array<ref LineElement> Xmldata = parser.GetElementString("type",itemType);
					LineElement elementData        = parser.GetElementStringValue(params.param1,Xmldata);
					if (elementData != null || elementData.data != string.Empty || elementData.index > -1)
					{
						parser.ReplaceElementValue(params.param1,elementData.index, params.param2);
					}
				}
				parser.SaveChanges();
				if (sender != null)
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(), "All Edits saved to: VPPAdminTools/Exports/types.xml",NotifyTypes.NOTIFY);
			}
		}
	}
	
	void GetScanInfo(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if ( type == CallType.Server )
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuXMLEditor", "", false)) return;
			
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[XMLEditor] Started a loot scan for item: " + data.param1));
			map<ref Object,vector> FoundItems = new map<ref Object,vector>;
			map<string,vector> ItemPositons = new map<string,vector>;
			array<vector> ScanPockets = GetScanPositions();
			int currentPosIndex = 0;
			foreach(vector PositionXYZ : ScanPockets)
			{
				Print("Started Scan at: "+ Vector(PositionXYZ[0], 0, PositionXYZ[1]).ToString());
				array<Object> objects = new array<Object>;
				GetGame().GetObjectsAtPosition(Vector(PositionXYZ[0], 0, PositionXYZ[1]), 1500, objects, NULL);
				
				for (int i = 0; i < objects.Count(); ++i)
				{
					EntityAI ent;
					if (Class.CastTo(ent, objects.Get(i)))
					{
						data.param1.ToLower();
						string entType = ent.GetType();
						entType.ToLower();

						if (entType == data.param1 && !FoundItems.Contains(ent))
						{
							FoundItems.Insert(ent, ent.GetPosition());
							ItemPositons.Insert(data.param1 +"_"+ i.ToString(), ent.GetPosition());
						}
					}
				}
				
				string formatted = string.Format("Scan Position %3/%4: [%2]\nFound Items: [%1]\n",FoundItems.Count(),Vector(PositionXYZ[0], 0, PositionXYZ[1]).ToString(),currentPosIndex,ScanPockets.Count());
				currentPosIndex++;
				Print("V++ AdminTools: "+ formatted);
			}
			
			if (sender != null)
			{
				Param1<ref map<string,vector>> m_Data = new Param1<ref map<string,vector>>(ItemPositons);
				GetRPCManager().SendRPC( "RPC_MenuXMLEditor", "HandleStats", m_Data, true, sender);
			}
		}
	}
	
	array<vector> GetScanPositions()
	{
		string cfg_Path = "CfgWorlds " + GetGame().GetWorldName() + " Names";
		int city_count = GetGame().ConfigGetChildrenCount( cfg_Path );
		array<string> cityNames = new array<string>;
		for( int i = 0; i < city_count; i++ )
		{
			string city_name;
			GetGame().ConfigGetChildName(cfg_Path, i, city_name );
			cityNames.Insert(city_name);
		}
		
		array<vector> positions = new array<vector>;
		foreach(string city : cityNames)
		{
			if (GetGame().ConfigIsExisting(cfg_Path + " " + city + " position"))
			{
				positions.Insert(GetGame().ConfigGetVector(cfg_Path + " " + city + " position"));
			}
		}
		return positions;
	}
};					