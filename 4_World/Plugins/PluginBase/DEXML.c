class DEXML
{
	/*
	nominal - items of this type are spawned to this value (assuming there is enough place in the world, CE queue is not pre-occupied and restock timer reaches 0)
	lifetime - time in seconds before this type gets deleted (assuming none touches it)
	restock - if set to 0, CE tries to respawn type in bulks, if set to positive value, it is time in seconds before ONE item of this type is spawned again
	min - if amount of items of this type reach below this value in world, they get spawned to nominal again (they follow restock logic though)
	quantmin - min % for quantity (rags,mags,ammo,..)
	quantmax - max % for quantity (rags,mags,ammo,..)
	cost - priority for CE (higher value should be prioritized by the spawner)
	*/
	const string DB_FILE_PATH   = "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\db\\";
	const string DB_FILE_SAVE_PATH   = "$profile:";
	const string DB_BACKUP_NAME = "types_bak.xml";
	const string DB_NOMINAL     = "nominal";
	const string DB_LIFETIME    = "lifetime";
	const string DB_RESTOCK     = "restock";
	const string DB_MIN         = "min";
	const string DB_QUANTMIN    = "quantmin";
	const string DB_QUANTMAX    = "quantmax";
	const string DB_COST        = "cost";

	const string m_TypeEnd   = "</type>";
	const string m_TypeStart = "<type";

	const string m_NominalStart  = "<nominal>";
	const string m_NominalEnd  = "</nominal>";

	const string m_LifetimeStart  = "<lifetime>";
	const string m_LifetimeEnd  = "</lifetime>";

	const string m_RestockStart  = "<restock>";
	const string m_RestockEnd  = "</restock>";

	const string m_MinStart  = "<min>";
	const string m_MinEnd  = "</min>";

	const string m_QuantminStart  = "<quantmin>";
	const string m_QuantminEnd  = "</quantmin>";

	const string m_QuantmaxStart  = "<quantmax>";
	const string m_QuantmaxEnd  = "</quantmax>";

	const string m_CostStart  = "<cost>";
	const string m_CostEnd  = "</cost>";

	ref array<string> m_XMLData = new array<string>;

	void DEXML()
	{
		GetRPCManager().AddRPC( "RPC_SetDataByField", "SetDataByField", this, SingeplayerExecutionType.Server );
	}

	void ~DEXML()
	{
		
	}

	void SetDataByField( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		//int = line Position, string field name, string new value
		Param3<int,string,string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	int line_Position = data.param1;
        	string field_name = data.param2;
        	string new_value  = data.param3;

        	if (line_Position)
        	{
				string SearchStr = m_XMLData.Get(line_Position);
				string NewValue;
				string SpacerStr = "        ";
				switch(field_name)
				{
					case DB_NOMINAL:
					if (SearchStr.Contains(m_NominalStart))
			    	{
			    		NewValue = SpacerStr + m_NominalStart + new_value + m_NominalEnd;
			    		m_XMLData.Set(data.param1,NewValue);
			    		Print("Updated nominal:"+NewValue);
			    	}
					break;

					case DB_LIFETIME:
					if (SearchStr.Contains(m_LifetimeStart))
			    	{
			    		NewValue = SpacerStr + m_LifetimeStart + new_value + m_LifetimeEnd;
			    		m_XMLData.Set(data.param1,NewValue);
			    		Print("Updated lifetime:"+NewValue);
			    	}
					break;

					case DB_RESTOCK:
					if (SearchStr.Contains(m_RestockStart))
			    	{
			    		NewValue = SpacerStr + m_RestockStart + new_value + m_RestockEnd;
			    		m_XMLData.Set(data.param1,NewValue);
			    		Print("Updated restock:"+NewValue);
			    	}
					break;

					case DB_MIN:
					if (SearchStr.Contains(m_MinStart))
			    	{
			    		NewValue = SpacerStr + m_MinStart + new_value + m_MinEnd;
			    		m_XMLData.Set(data.param1,NewValue);
			    		Print("Updated min:"+NewValue);
			    	}
					break;

					case DB_QUANTMIN:
					if (SearchStr.Contains(m_QuantminStart))
			    	{
			    		NewValue = SpacerStr + m_QuantminStart + new_value + m_QuantminEnd;
			    		m_XMLData.Set(data.param1,NewValue);
			    		Print("Updated quantmin:"+NewValue);
			    	}
					break;

					case DB_QUANTMAX:
					if (SearchStr.Contains(m_QuantmaxStart))
			    	{
			    		NewValue = SpacerStr + m_QuantmaxStart + new_value + m_QuantmaxEnd;
			    		m_XMLData.Set(data.param1,NewValue);
			    		Print("Updated quantmax:"+NewValue);
			    	}
					break;

					case DB_COST:
					if (SearchStr.Contains(m_CostStart))
			    	{
			    		NewValue = SpacerStr + m_CostStart + new_value + m_CostEnd;
			    		m_XMLData.Set(data.param1,NewValue);
			    		Print("Updated cost:"+NewValue);
			    	}
					break;
				}
				//Check for if back is there write if not
				if (!FileExist("$profile:" + DB_BACKUP_NAME)) { WriteBackUp(); }

				FileHandle currentFile = OpenFile( DB_FILE_SAVE_PATH + "types.xml", FileMode.WRITE);
				foreach(string Strline : m_XMLData)
				{
					FPrintln(currentFile,Strline);
				}
			    CloseFile(currentFile);
			}
  		}
	}

	void WriteBackUp()
	{
		autoptr array<string> m_Content = new array<string>;
		string line_content;
		FileHandle currentFile = OpenFile( DB_FILE_PATH + "types.xml", FileMode.READ);
		if (currentFile != 0)
		{
			Print("Collecting old types.xml for a backup");
			while ( FGets(currentFile,line_content) > 0 )
			{
				m_Content.Insert(line_content);
			}
			CloseFile(currentFile);
		}

		currentFile = OpenFile( "$profile:" + DB_BACKUP_NAME, FileMode.WRITE);
		if (currentFile != 0)
		{
			foreach(string strLine : m_Content)
			{
				FPrintln(currentFile,strLine);
			}
		    CloseFile(currentFile);
		    Print("New Types.xml backup created!");
		}
	}

	void GetDataFromField(int StartPos, int EndPos, string FieldName, PlayerIdentity sender)
	{
		Param3<int,string,string> map_param;
	    for (StartPos; StartPos < EndPos; ++StartPos)
	    {
	    	string SearchStr = m_XMLData.Get(StartPos);
	    	switch(FieldName)
	    	{
	    		case DB_NOMINAL:
	    		if (SearchStr.Contains(m_NominalStart))
		    	{
		    		SearchStr.Replace(m_NominalStart,"");
		    		SearchStr.Replace(m_NominalEnd,"");
		    		if (sender != NULL)
				    {
				    	map_param = new Param3<int,string,string>(StartPos,SearchStr,DB_NOMINAL);
			        	GetRPCManager().SendRPC( "RPC_DEXML_UI", "SortData", map_param, true, sender);
				    }
		    		Print("Found nominal:"+SearchStr);
		    	}
	    		break;

	    		case DB_LIFETIME:
	    		if (SearchStr.Contains(m_LifetimeStart))
		    	{
		    		SearchStr.Replace(m_LifetimeStart,"");
		    		SearchStr.Replace(m_LifetimeEnd,"");
		    		if (sender != NULL)
				    {
				    	map_param = new Param3<int,string,string>(StartPos,SearchStr,DB_LIFETIME);
			        	GetRPCManager().SendRPC( "RPC_DEXML_UI", "SortData", map_param, true, sender);
				    }
		    		Print("Found lifetime:"+SearchStr);
		    	}
	    		break;

	    		case DB_RESTOCK:
	    		if (SearchStr.Contains(m_RestockStart))
		    	{
		    		SearchStr.Replace(m_RestockStart,"");
		    		SearchStr.Replace(m_RestockEnd,"");
		    		if (sender != NULL)
				    {
				    	map_param = new Param3<int,string,string>(StartPos,SearchStr,DB_RESTOCK);
			        	GetRPCManager().SendRPC( "RPC_DEXML_UI", "SortData", map_param, true, sender);
				    }
		    		Print("Found restock:"+SearchStr);
		    	}
	    		break;

	    		case DB_MIN:
	    		if (SearchStr.Contains(m_MinStart))
		    	{
		    		SearchStr.Replace(m_MinStart,"");
		    		SearchStr.Replace(m_MinEnd,"");
		    		if (sender != NULL)
				    {
				    	map_param = new Param3<int,string,string>(StartPos,SearchStr,DB_MIN);
			        	GetRPCManager().SendRPC( "RPC_DEXML_UI", "SortData", map_param, true, sender);
				    }
		    		Print("Found min:"+SearchStr);
		    	}
	    		break;

	    		case DB_QUANTMIN:
	    		if (SearchStr.Contains(m_QuantminStart))
		    	{
		    		SearchStr.Replace(m_QuantminStart,"");
		    		SearchStr.Replace(m_QuantminEnd,"");
		    		if (sender != NULL)
				    {
				    	map_param = new Param3<int,string,string>(StartPos,SearchStr,DB_QUANTMIN);
			        	GetRPCManager().SendRPC( "RPC_DEXML_UI", "SortData", map_param, true, sender);
				    }
		    		Print("Found quantmin:"+SearchStr);
		    	}
	    		break;

	    		case DB_QUANTMAX:
	    		if (SearchStr.Contains(m_QuantmaxStart))
		    	{
		    		SearchStr.Replace(m_QuantmaxStart,"");
		    		SearchStr.Replace(m_QuantmaxEnd,"");
		    		if (sender != NULL)
				    {
				    	map_param = new Param3<int,string,string>(StartPos,SearchStr,DB_QUANTMAX);
			        	GetRPCManager().SendRPC( "RPC_DEXML_UI", "SortData", map_param, true, sender);
				    }
		    		Print("Found quantmax:"+SearchStr);
		    	}
	    		break;

	    		case DB_COST:
	    		if (SearchStr.Contains(m_CostStart))
		    	{
		    		SearchStr.Replace(m_CostStart,"");
		    		SearchStr.Replace(m_CostEnd,"");
		    		if (sender != NULL)
				    {
				    	map_param = new Param3<int,string,string>(StartPos,SearchStr,DB_COST);
			        	GetRPCManager().SendRPC( "RPC_DEXML_UI", "SortData", map_param, true, sender);
				    }
		    		Print("Found cost:"+SearchStr);
		    	}
	    		break;
	    	}
	    }
	}

	void Parse(string ItemClassName, string FeildCatagory, PlayerIdentity sender)
	{
		int LinePos    = 0;
		int StartPos   = -1;
		int EndPos     = 0;
		bool Proceed   = false;
		bool Finished  = false;
	    for (LinePos; LinePos < m_XMLData.Count(); ++LinePos)
	    {
	    	if (!Finished)
	    	{
		    	string line_content = m_XMLData.Get(LinePos);

		    	if (line_content != "")
				{
					if (line_content.Contains(m_TypeStart) && !line_content.Contains(m_TypeEnd) && !line_content.Contains("types"))
					{
						if (line_content.Contains(ItemClassName))
						{
							StartPos = LinePos;
							Proceed = true;
							Print("POS: " + LinePos + " Class Start:"+line_content);
						}
					}
					else if (line_content.Contains(m_TypeEnd) && !line_content.Contains(m_TypeStart) && !line_content.Contains("types"))
					{
						if (Proceed)
						{
							Finished = true;
							EndPos = LinePos;
							Print("POS: " + LinePos + " Class End"+line_content);
							GetDataFromField(StartPos,EndPos,FeildCatagory,sender);
						}
					}
				}
			}
	    }

	    //Send Error item not found
	    if (!Proceed)
	    {
	    	Param3<int,string,string> map_param = new Param3<int,string,string>(-1,"error","error");
			GetRPCManager().SendRPC( "RPC_DEXML_UI", "SortData", map_param, true, sender);
			Print("Error loading item from Types.xml");
	    }
	    Proceed  = false;
		LinePos  = 0;
		StartPos = -1;
		EndPos   = 0;
	}

	void ReadFromXML(string ItemClassName, string FieldCatagory, PlayerIdentity sender)
	{
		string line_content;
		FileHandle currentFile;
		if (FileExist("$profile:types.xml"))
		{
			currentFile = OpenFile( "$profile:types.xml", FileMode.READ);
			m_XMLData.Clear();
			if (currentFile != 0)
			{
				while ( FGets(currentFile,line_content) > 0 )
				{
					m_XMLData.Insert(line_content);
				}
				CloseFile(currentFile);
			}
			Parse(ItemClassName,FieldCatagory,sender);
		} else {
			
			currentFile = OpenFile( DB_FILE_PATH + "types.xml", FileMode.READ);
			m_XMLData.Clear();
			if (currentFile != 0)
			{
				while ( FGets(currentFile,line_content) > 0 )
				{
					m_XMLData.Insert(line_content);
				}
				CloseFile(currentFile);
			}
			Parse(ItemClassName,FieldCatagory,sender);
		}
	}
}