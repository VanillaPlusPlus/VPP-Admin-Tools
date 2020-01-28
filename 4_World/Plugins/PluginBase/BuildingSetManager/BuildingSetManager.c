class BuildingSetManager : ConfigurablePlugin
{
   private ref array<ref BuildingSet> m_BuildingSets;

   void BuildingSetManager()
   {
      JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/BuildingSetManager/";
      m_BuildingSets = new array<ref BuildingSet>;

      /* RPCs */
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "GetBuildingSets", this);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "GetSetData", this);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteAddNewSet", this);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteUpdateSet", this);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteDeleteSet", this);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteSaveEdits", this);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteQuickDeleteObject", this);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "ExportSetToCode", this);
      //-------
   }

   override void OnInit()
   {
		Load();
		
		//Test
		/*for(int i = 0; i < 1; i++)
		{
			string setName  = "BuildingSet" +i;
			AddBuildingSet(setName, true);
			for(int x = 0; x < 1; x++)
			{
				AddBuildingToSet(setName, "land_container_1moh", "4353.2402 27.91 3178.1599", "0 0 0", true);
			}
		}*/
   }
	
   override void Load()
   {
      ref array<string> paths = new array<string>;
      GetFilePaths(paths);

      if(paths.Count() == 0)
      {
         MakeDirectory(JSONPATH);
         return;
      }

      ref BuildingSet bSet;

      foreach(string path : paths)
      {
         bSet = null;
		
         if(FileExist(path))
         {
			autoptr FileSerializer file = new FileSerializer();
			if (file.Open(path, FileMode.READ))
			{
			    file.Read(bSet);
			    file.Close();
				if(bSet)
				{
					if(m_BuildingSets.Find(bSet) < 0)
					{
						m_BuildingSets.Insert(bSet);
					}
				}
			}
         }
      }
      SpawnActiveBuildings();
   }
	
	void SaveBuildingSet(BuildingSet bSet)
	{
		autoptr FileSerializer file = new FileSerializer();
		string path = JSONPATH + bSet.GetName() + ".vpp";
		if (file.Open(path, FileMode.WRITE))
		{
		    file.Write(bSet);
		    file.Close();
			GetSimpleLogger().Log("[BuildingSetManager] SaveBuildingSet() Saved:"+path);
			return;
		}
		GetSimpleLogger().Log("[BuildingSetManager] SaveBuildingSet() FALIED To Save:"+path);
	}
	
	//use only when you edit an exisitng building set name (to delete old set file)
	void DeleteSavedFile(string fileName)
	{
		if (FileExist(JSONPATH + fileName + ".vpp"))
		{
			DeleteFile(JSONPATH + fileName + ".vpp");
			GetSimpleLogger().Log("[BuildingSetManager] DeleteSavedFile() Deleted file: "+fileName);
			return;
		}
		GetSimpleLogger().Log("[BuildingSetManager] DeleteSavedFile() File  not found: "+fileName);
	}
	
	array<string> GetSetsNames()
	{
		autoptr array<string> setNames = new array<string>;
			foreach(BuildingSet buildingSet: m_BuildingSets)
				setNames.Insert(buildingSet.GetName());

		return setNames;
	}
	
   /*
      [RPC's]
   */
  	void GetBuildingSets(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager")) return;

			GetRPCManager().SendRPC("RPC_MenuObjectManager","HandleData", new Param1<ref array<string>>(GetSetsNames()),true,sender);
		}
	}
	
	void GetSetData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager")) return;
			
			autoptr BuildingSet buildSet = GetBuildingSetByName(data.param1);
			if (buildSet == null){
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Error, Building Set: "+data.param1+"\nwas not found!",NotifyTypes.ERROR);
				return;
			}
			
			GetRPCManager().SendRPC("RPC_MenuObjectManager","HandleSetsData", new Param1<ref BuildingSet>(buildSet),true,sender);
			GetSimpleLogger().Log("[BuildingSetManager] GetSetData() Sending set to user "+sender.GetPlainId() + " name: "+sender.GetName());
		}
	}
	
	void RemoteAddNewSet(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string,bool> data;
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager:CreateNewSet")) return;
			
			AddBuildingSet(data.param1, data.param2);
			
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Added New Building set: "+data.param1,NotifyTypes.NOTIFY);
			GetSimpleLogger().Log("[BuildingSetManager] RemoteAddNewSet() Added new buildingset by user "+sender.GetPlainId() + " name: "+sender.GetName());
			autoptr AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] RemoteAddNewSet() Added new buildingset: " + data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, rpt);
			//Send new data to client
			GetRPCManager().SendRPC("RPC_MenuObjectManager","HandleData", new Param1<ref array<string>>(GetSetsNames()),true,sender);
		}
	}
	
	void RemoteUpdateSet(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<string,string,bool> data;
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager:UpdateSet")) return;
			
			autoptr BuildingSet toEdit = GetBuildingSetByName(data.param1);
			if (toEdit != null)
			{
				toEdit.SetActive(data.param3);
				toEdit.SetName(data.param2);
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Updated Building set: "+data.param1 + " to new name\n"+data.param2,NotifyTypes.NOTIFY);
				GetSimpleLogger().Log("[BuildingSetManager] RemoteUpdateSet() Updated buildingset by user "+sender.GetPlainId() + " name: "+sender.GetName());
				DeleteSavedFile(data.param1); //Delete old sets save file (nothing to do with memeory as this file is not loaded)
				SaveBuildingSet(toEdit);
				//Send new data to client
				GetRPCManager().SendRPC("RPC_MenuObjectManager","HandleData", new Param1<ref array<string>>(GetSetsNames()),true,sender);
				autoptr AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] RemoteUpdateSet() Updated buildingset: " + data.param1);
				GetWebHooksManager().PostData(AdminActivityMessage, rpt);
				return;
			}
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Something went wrong! Unable to update set",NotifyTypes.NOTIFY);
		}
	}
	
	void RemoteDeleteSet(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager:DeleteSet")) return;
			
			RemoveBuildingSet(data.param1);
			GetSimpleLogger().Log("[BuildingSetManager] RemoteDeleteSet() Deleted buildingset by user "+sender.GetPlainId() + " name: "+sender.GetName());
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Deleted building set: "+data.param1,NotifyTypes.NOTIFY);
			//Send new data to client
			GetRPCManager().SendRPC("RPC_MenuObjectManager","HandleData", new Param1<ref array<string>>(GetSetsNames()),true,sender);
			autoptr AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] RemoteDeleteSet() Deleted buildingset: " + data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, rpt);
			return;
		}
		GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Something went wrong! Unable to delete set!",NotifyTypes.NOTIFY);
	}
	
	void RemoteSaveEdits(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<ref array<ref SpawnedBuilding>,string> data;
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager:EditSet")) return;
		
			autoptr BuildingSet bSet = GetBuildingSetByName(data.param2);
			if (bSet != null)
			{
				bool wasActive = bSet.GetActive();
				bSet.SetActive(false);
				bSet.ClearBuildings();
				bSet.SetBuildingsArray(data.param1);
				
				SaveBuildingSet(bSet);
				bSet.SetActive(wasActive);
			}
			
			//Send new data to client
			GetRPCManager().SendRPC("RPC_MenuObjectManager","HandleData", new Param1<ref array<string>>(GetSetsNames()),true,sender);
			GetSimpleLogger().Log("[BuildingSetManager] RemoteSaveEdits() Saving & Reloading Building Set:" + bSet.GetName());
			autoptr AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] RemoteSaveEdits() Saving & Reloading Building Set:" + bSet.GetName());
			GetWebHooksManager().PostData(AdminActivityMessage, rpt);
			return;
		}
		GetSimpleLogger().Log("[BuildingSetManager] RemoteSaveEdits() Error saving set: "+bSet.GetName());
	}
	
	void RemoteQuickDeleteObject(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<Object> data;
		if (!ctx.Read(data)) return;
		
		if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager")) return;
		
		if (data.param1 != null)
			GetGame().ObjectDelete(data.param1);
	}
	
	void ExportSetToCode(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		autoptr BuildingSet bset = GetBuildingSetByName(data.param1);
		if (bset != null)
		{
			bset.ExportBuildings();
			autoptr AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] ExportSetToCode() Exported Building Set:" + data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, rpt);
		}
	}
   /*
      [End RPC's]
   */
	
   void SpawnActiveBuildings()
   {
      foreach(BuildingSet bSet : m_BuildingSets)
      {
         if(bSet.GetActive())
         {
            autoptr array<ref SpawnedBuilding> buildings;
            bSet.GetSpawnedBuildings(buildings);

            foreach(SpawnedBuilding building : buildings)
            {
               if(building.GetActive())
               {
                  building.SpawnObject();
                  Print("Spawning Building ["+building.GetName()+"]");
               }
            }  
         }
      }
   }

   void AddBuildingSet(string name, bool active = false)
   {
      ref BuildingSet bSet = new BuildingSet(name, active);
      m_BuildingSets.Insert(bSet);
		SaveBuildingSet(bSet);
   }

   void AddBuildingToSet(string bSetName, string building, vector pos, vector orientation, bool active = false)
   {
      autoptr BuildingSet bSet = GetBuildingSetByName(bSetName);
		
		if(bSet == null)
			Print("[BuildingSet]:: AddBuildingToSet(): Invalid Set Name: " + bSetName);
		
      if(bSet != null)
      {
         bSet.AddBuilding(building, pos, orientation, active);
			SaveBuildingSet(bSet);
      }
   }

   void EnableAllBuildingSets()
   {
      foreach(BuildingSet bSet : m_BuildingSets)
      {
         bSet.SetActive(true);
			SaveBuildingSet(bSet);
      }
   }

   void RemoveBuildingSet(string bSetName)
   {
      ref BuildingSet bSet = GetBuildingSetByName(bSetName);

      if(bSet)
      {
         DeleteFile(JSONPATH + bSet.GetName() + ".vpp");
         m_BuildingSets.RemoveItem(bSet);
         delete bSet;
      }
   }

   void DisableAllBuildingSets()
   {
      foreach(BuildingSet bSet : m_BuildingSets)
      {
         bSet.SetActive(false);
			SaveBuildingSet(bSet);
      }
   }

   void DeleteBuildingByName(string bSetName, string buildingName)
   {
      autoptr BuildingSet building = GetBuildingSetByName(bSetName);

      if(building)
      {
         building.DeleteBuildingByName(buildingName);
      }
		
		SaveBuildingSet(building);
   }

   void ToggleBuildingByName(string bSetName, string buildingName, bool active = false)
   {
      autoptr BuildingSet building = GetBuildingSetByName(bSetName);

      if(building)
      {
         building.SetBuildingActive(buildingName, active);
      }
		SaveBuildingSet(building);
   }

   BuildingSet GetBuildingSetByName(string name)
   {
      foreach(BuildingSet bSet : m_BuildingSets)
      {
         if(bSet.GetName() == name)
         {
            return bSet;
         }
      }
      return null;
   }
}

BuildingSetManager GetBuildingSetManager()
{
	return BuildingSetManager.Cast(GetPluginManager().GetPluginByType(BuildingSetManager));
}