class XMLEditor extends PluginBase
{
	private ref array<string> ELEMENTS = { "nominal", "lifetime", "restock", "min", "quantmin", "quantmax", "cost" };
	private string XML_PATH_BACKUP 	= "$profile:VPPAdminTools/Backups/";
	private string MISSION_XML_PATH = "$mission:db/types.xml";
	
	void XMLEditor()
	{
		/* RPCs */
		GetRPCManager().AddRPC( "RPC_XMLEditor", "GetDetails", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC( "RPC_XMLEditor", "EditElemets", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC( "RPC_XMLEditor", "GetScanInfo", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC( "RPC_XMLEditor", "GetTypesFiles", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC( "RPC_XMLEditor", "GetTypesFromFile", this, SingeplayerExecutionType.Server);
	}
	
	void GetDetails(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string, string> data; //object type, xml file
		if (!ctx.Read(data)) return;
		
	 	if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuXMLEditor", "", false)) return;
			
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[XMLEditor] Requested xml data for item: " + data.param1));

			XMLParser parser = new XMLParser(data.param2);
			if (parser.Load() && parser.VerifyXml())
			{
				array<ref Param3<string,string,int>> result = new array<ref Param3<string,string,int>>;
				
				foreach(string elementType : ELEMENTS)
				{
					array<ref LineElement> Xmldata = parser.GetElementString("type", data.param1);
					LineElement elementData        = parser.GetElementStringValue(elementType, Xmldata);
					if (elementData == null || elementData.data == "" || elementData.index <= -1)
						result.Insert(new Param3<string,string,int>(elementType,"#VSTR_NOT_DEFINED",-1));
					else
						result.Insert(new Param3<string,string,int>(elementType, elementData.data, elementData.index));
				}
				if (sender != null)
					GetRPCManager().VSendRPC("RPC_MenuXMLEditor", "HandleDetails", new Param2<string, ref array<ref Param3<string,string,int>>>(data.param1,result), true, sender);
			}
		}
	}
	
	void EditElemets(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<string, string, ref array<ref Param3<string,string,int>>> data; //file path, object type, data
		if (!ctx.Read(data)) return;
		
	 	if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuXMLEditor", "", false)) return;
			
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[XMLEditor] Saved edits to type.xml"));

			array<ref Param3<string,string,int>> paramsData = data.param3;
			string itemType = data.param2;
			
			XMLParser parser = new XMLParser(data.param1); //xml file
			if (parser.Load() && parser.VerifyXml())
			{
				foreach(Param3<string,string,int>> params: paramsData)
				{
					array<ref LineElement> Xmldata = parser.GetElementString("type", itemType);
					LineElement elementData        = parser.GetElementStringValue(params.param1, Xmldata);
					if (elementData != null || elementData.data != string.Empty || elementData.index > -1)
					{
						parser.ReplaceElementValue(params.param1,elementData.index, params.param2);
					}
				}
				CreateBackup(data.param1);//create backup before saving (saved in $profile)
				parser.SaveChanges(data.param1);
				if (sender != null)
				{
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(), "All Edits saved to: " + data.param1, NotifyTypes.NOTIFY);
				}
			}
		}
	}
	
	void GetScanInfo(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string,float> data;
		if (!ctx.Read(data)) return;
		
		if ( type == CallType.Server )
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuXMLEditor", "", false)) return;
			
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[XMLEditor] Started a loot scan for item: " + data.param1));
			map<string,vector> ItemPositons = new map<string,vector>;
			array<EntityAI> entities = new array<EntityAI>;

			string typeTosearch = data.param1;
			float radius = data.param2;

			vector mapRadius = Vector(radius, 1200.0, radius);

			typename t = typeTosearch.ToType();
			if (t && (t.IsInherited(CrashBase) || t.IsInherited(House) || t.IsInherited(BuildingSuper)))
			{
				//Land_ / House hybrid type search (SLOW!)
				DayZPlayerUtils.PhysicsGetEntitiesInBox("0.0 -1200.0 0.0", mapRadius, entities);
			}else{
				DayZPlayerUtils.SceneGetEntitiesInBox("0.0 -1200.0 0.0", mapRadius, entities);
			}

			for (int i = 0; i < entities.Count(); ++i)
			{
				EntityAI ent = entities[i];
				if (ent)
				{
					data.param1.ToLower();
					string entType = ent.GetType();
					entType.ToLower();
					if (entType == data.param1)
						ItemPositons.Insert(data.param1 +"_"+ i.ToString(), ent.GetPosition());
				}
			}
			
			Print(string.Format("[VPPAdminTools] Found Items: [%1]",entities.Count()));

			if (sender != null)
			{
				Param1<ref map<string,vector>> m_Data = new Param1<ref map<string,vector>>(ItemPositons);
				GetRPCManager().VSendRPC("RPC_MenuXMLEditor", "HandleStats", m_Data, true, sender);
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

	void CreateBackup(string originalPath)
	{
		int hour, minute, second;
		int year, month, day;
		
		GetHourMinuteSecondUTC(hour,minute,second);
		GetYearMonthDayUTC(year,month,day);

		string fileName = originalPath;
		fileName.Replace("$mission:", "");
		fileName.Replace("/", "_");
		fileName.Replace(".xml", "");

		string timeStamp = string.Format("%1-%2-%3_%4-%5-%6",year.ToString(),month.ToString(),day.ToString(),hour.ToString(),minute.ToString(),second.ToString());
		PluginFileHandler.FileDuplicate(originalPath, XML_PATH_BACKUP + fileName + "_" + timeStamp + ".xml");
	}

	void GetTypesFiles(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
		{
			if (!sender) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuXMLEditor", "", false)) return;

			map<string, string> typesFiles = new map<string, string>;
			typesFiles.Insert(MISSION_XML_PATH, "types.xml (root)");
			FindXmlFiles(typesFiles);

			if (typesFiles.Count() > 0)
			{
				GetRPCManager().VSendRPC("RPC_MenuXMLEditor", "HandleTypesFiles", new Param1<ref map<string, string>>(typesFiles), true, sender);
			}
		}
	}

	void GetTypesFromFile(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
		{
			if (!sender) return;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuXMLEditor", "", false)) return;

			Param1<string> data;
			if (!ctx.Read(data)) return;

			CF_XML_Document document;
			if (!CF.XML.ReadDocument(data.param1, document))
			{
				Print("[VPPAdminTools] GetTypesFromFile :: Failed to read xml! File not found or syntax is broken.");
				return;
			}

			CF_XML_Tag root_tag = document.Get("types")[0];
			if (root_tag && root_tag.GetName() == "types")
			{
				array<string> classTypes = {};
				CF_XML_Element content = root_tag.GetContent();
				for(int i = 0; i < content.Count(); ++i)
				{
					CF_XML_Tag tag = content[i];
					if (!tag || tag.GetName() != "type")
						continue;
					
					CF_XML_Attribute att = tag.GetAttribute("name");
					if (!att)
						continue;

					classTypes.Insert(att.ValueAsString());
				}
				if (classTypes.Count() > 0){
					GetRPCManager().VSendRPC("RPC_MenuXMLEditor", "HandleTypesList", new Param1<ref array<string>>(classTypes), true, sender);
				}
			}
		}
	}

	/*
	* Parse thru and find any extra types files added via cfgeconomycore.xml
	*/
	void FindXmlFiles(inout map<string, string> typesFiles)
	{
		CF_XML_Document document;
		if (!CF.XML.ReadDocument("$mission:cfgeconomycore.xml", document))
		{
			Print("[VPPAdminTools] FindXmlFiles :: Failed to read cfgeconomycore.xml! File not found.");
			return;
		}

		CF_XML_Tag root_tag = document.Get("economycore")[0];
		if (!root_tag || root_tag.GetName() != "economycore")
		{
			Print("[VPPAdminTools] FindXmlFiles :: Failed to read cfgeconomycore.xml! File Syntax/content is broken. Unable to find economycore element");
			return;
		}

		CF_XML_Element content = root_tag.GetContent();
		for(int i = 0; i < content.Count(); ++i)
		{
			CF_XML_Tag tag = content[i];
			if (!tag || tag.GetName() != "ce")
				continue;
			
			CF_XML_Attribute att = tag.GetAttribute("folder");
			string dirPath = att.ValueAsString();
			//content
			CF_XML_Element filePaths = tag.GetContent();
			if (filePaths && filePaths.Count() > 0)
			{
				for(int x = 0; x < filePaths.Count(); ++x)
				{
					CF_XML_Tag _tag = filePaths[x];
					if (!_tag)
						continue;
					
					CF_XML_Attribute _att = _tag.GetAttribute("type");
					if (!_att || _att.ValueAsString() != "types")
						continue;
					
					_att = _tag.GetAttribute("name");
					//construct file dirPath
					string finalPath = string.Format("$mission:%1/%2", dirPath, _att.ValueAsString());
					typesFiles.Insert(finalPath, _att.ValueAsString());
				}
			}
		}
	}
};					