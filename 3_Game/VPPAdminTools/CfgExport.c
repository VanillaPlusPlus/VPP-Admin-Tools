class VPPATCfgExport
{
	static void DumpPairsToLogs()
	{
		map<string,string> staticObjs; //class type, model path
		map<string,string> bldrObjs;

		map<string,string> counterPart = new map<string,string>; //bldr_ type , staticobj_ counter part

		DumpStaticObjects("staticobj_", staticObjs);
		DumpStaticObjects("bldr_", bldrObjs);

		Print("Total StaticObj_ ► " + staticObjs.Count());
		Print("Total bldr_ ► " + bldrObjs.Count());
		
		for (int i = 0; i < bldrObjs.Count(); ++i)
		{
			string staticObj_type;
			if (!staticObjs.GetKeyByValueChecked(bldrObjs.GetElement(i), staticObj_type)) //very slow lol
				continue;

			Print(bldrObjs.GetKey(i) + " | " + staticObj_type);
		}
	}

	static void DumpStaticObjects(string wildCard, out map<string,string> objTypes)
	{
		objTypes = new map<string,string>;
        wildCard.ToLower();

        string Config_Path = "CfgVehicles";
        int totalClasses = g_Game.ConfigGetChildrenCount(Config_Path);

        for (int i = 0; i < totalClasses; ++i)
        {
            string objType;
            if (!GetGame().ConfigGetChildName(Config_Path, i, objType))
            	continue;

            string base_name;
            bool baseType = GetGame().ConfigGetBaseName(string.Format("%1 %2",Config_Path, objType), base_name);
            base_name.ToLower();

        	if (base_name != "housenodestruct")
            	continue;

            objType.ToLower();

            if (!objType.Contains(wildCard)) 
                continue;

            //model path
            string modelPath = string.Empty;
            if (!GetGame().ConfigGetTextRaw(string.Format("%1 %2 model", Config_Path, objType), modelPath))
            	continue;

            modelPath.ToLower();
            if (modelPath[0] == "\\")
            	modelPath[0] = string.Empty;

            objTypes.Insert(objType, modelPath);
        }
	}
};