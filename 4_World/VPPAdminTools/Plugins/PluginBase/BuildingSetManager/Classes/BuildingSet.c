class BuildingSet
{
   private string m_Name;
   private bool   m_IsActive;
   private ref array<ref SpawnedBuilding> m_Buildings;

   void BuildingSet(string name, bool isAlive = false)
   {
      m_IsActive = isAlive;
      m_Name = name;
      m_Buildings = new array<ref SpawnedBuilding>;
   }

   void ~BuildingSet()
   {
      foreach(SpawnedBuilding building : m_Buildings)
      {
		if (building != null)
         	delete building;
      }
   }

   string GetName()
   {
      return m_Name;
   }
	
   void SetName(string newname)
   {
	  m_Name = newname;
   }

   bool GetActive()
   {
      return m_IsActive;
   }

   void GetSpawnedBuildings(inout array<ref SpawnedBuilding> buildings)
   {
      buildings = m_Buildings;
   }

   void DeleteBuildingByName(string name)
   {
      SpawnedBuilding building = GetBuildingByName(name);

      if(building)
      {
         m_Buildings.RemoveItem(building);
         delete building;
      }
   }

   void SetBuildingsArray(array<ref SpawnedBuilding> data)
   {
	  m_Buildings = data;
   }

   void AddBuilding(string name, vector pos, vector orientation, bool active)
   {
      m_Buildings.Insert(new SpawnedBuilding(name, pos, orientation, active));
   }
   
   //use only for client when creating new objects
   SpawnedBuilding AddBuildingObject(string name, vector pos, vector orientation, bool active, Object activeObj)
   {
		SpawnedBuilding result = new SpawnedBuilding(name, pos, orientation, active, activeObj);
		m_Buildings.Insert(result);
		return result;
   }

   void SetBuildingActive(string name, bool active)
   {
      SpawnedBuilding building = GetBuildingByName(name);

      if(building)
         building.SetActive(active);

      if(!building)
         Print("[BuildingSetManager]:: SetBuildingActive: " + name + " is an invalid name.");
   }

	void SetActive(bool active)
	{
		m_IsActive = active;
		
		foreach(SpawnedBuilding building : m_Buildings)
		{
			if (building != null)
			{
				if (m_IsActive)
					building.SetActive(building.GetActive());
				else
		         	building.DestroySpawnedEntity();
			}
		}
	}

   array<ref SpawnedBuilding> GetBuildings()
   {
	 return m_Buildings;
   }

   void ClearBuildings()
   {
	    foreach(SpawnedBuilding buidling : m_Buildings){
			if (buidling)
				delete buidling;
		}
		m_Buildings = new array<ref SpawnedBuilding>;
   }
	
	void RemoveBuilding(SpawnedBuilding instance)
	{
		int index = m_Buildings.Find(instance);
		if (index > -1 && m_Buildings.Get(index) != null)
		{
			m_Buildings.RemoveItem(instance);
			delete instance;
		}
	}
	
	//Only use for client, to update all objects within set (pos,orientation)
	void UpdateBuildingsData()
	{
		foreach(SpawnedBuilding building : m_Buildings)
		{
			bool networkObj = false;
			Object trackerObj = building.GetObject();
			string networkID = building.GetNetworkId();
			if (trackerObj == null)
			{
				//Find object using network ID
				TStringArray strs = new TStringArray;
				networkID.Split( ",",strs );
				trackerObj = GetGame().GetObjectByNetworkId(strs[1].ToInt(), strs[0].ToInt()); //low,high
				networkObj = true;
			}
			
			if (trackerObj == null) continue;
			building.SetPosition(trackerObj.GetPosition());
			building.SetOrientation(trackerObj.GetOrientation());
			
			if (!networkObj)
				GetGame().ObjectDelete(trackerObj);
		}
	}
	
	void ExportBuildings()
	{
		FileHandle m_FileHandle;
		m_FileHandle = OpenFile("$profile:VPPAdminTools/Exports/"+m_Name+".txt", FileMode.WRITE);
		if (m_FileHandle == 0) return;

		string function = "void SpawnObject(string objType, vector objPos, vector objOrientation)\n";
		function 		+= "{\n";
		function 		+= "\tObject m_Building = Object.Cast(GetGame().CreateObject(objType, objPos));\n";
		function 		+= "\tif (m_Building == null) return;\n";
		function 		+= "\tm_Building.SetAffectPathgraph(true, false);\n";
		function 		+= "\tGetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().UpdatePathgraphRegionByObject, 100, false, m_Building);\n";
		function 		+= "\tm_Building.SetPosition(objPos);\n";
		function 		+= "\tm_Building.SetOrientation(objOrientation);\n";
		function 		+= "}\n\n\n";
		
		FPrintln(m_FileHandle, function);
		
		foreach(SpawnedBuilding building: m_Buildings)
		{
			string format = "SpawnObject( \"" + building.GetTypeName() + "\", \"" + building.GetPosition().ToString(false) + "\", \"" + building.GetOrientation().ToString(false) + "\" );";
			FPrintln(m_FileHandle, format);
		}
		
		CloseFile(m_FileHandle);
	}

	SpawnedBuilding GetSpawnedBuilding(SpawnedBuilding compare)
	{
	  foreach(SpawnedBuilding building : m_Buildings)
      {
         if(building.GetName() == compare.GetName() || building == compare)
         {
            return building;
         }
      }
      return null;
	}
	
   SpawnedBuilding GetBuildingByName(string name)
   {
      foreach(SpawnedBuilding building : m_Buildings)
      {
         if(building.GetName() == name)
         {
            return building;
         }
      }
      return null;
   }
	
   SpawnedBuilding GetBuildingByType(Object compare)
   {
	  foreach(SpawnedBuilding building : m_Buildings)
      {
         if(building.GetObject() == compare)
         {
            return building;
         }
      }
      return null;
   }
	
   SpawnedBuilding GetBuildingByNetworkID(string id)
   {
	  foreach(SpawnedBuilding building : m_Buildings)
      {
         if(building.GetNetworkId() == id)
         {
            return building;
         }
      }
      return null;
   }
}