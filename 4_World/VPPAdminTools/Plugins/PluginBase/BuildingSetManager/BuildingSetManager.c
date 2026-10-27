class BuildingSetManager : ConfigurablePlugin
{
   private ref array<ref BuildingSet> m_BuildingSets;
	// where every set file is (journal + CF.FindFileEx folder scan, DayZ 1.30 FindFile workaround): VPPBuildingSetJournal
	[NonSerialized()]
	protected ref VPPBuildingSetJournal m_Journal;
	// Load(): the set ConvertLegacySet read, and the format counts for the summary line
	[NonSerialized()]
	protected ref BuildingSet m_ConvertedSet;
	[NonSerialized()]
	protected int m_ConvertedCount;
	[NonSerialized()]
	protected int m_UpgradedCount;

   void BuildingSetManager()
   {
      JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/BuildingSetManager/";
      m_BuildingSets = new array<ref BuildingSet>;
		m_Journal = new VPPBuildingSetJournal();
		m_Journal.SetFolder(JSONPATH);

      /* RPCs */
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "GetBuildingSets", this, SingleplayerExecutionType.Server);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "GetSetData", this, SingleplayerExecutionType.Server);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteAddNewSet", this, SingleplayerExecutionType.Server);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteUpdateSet", this, SingleplayerExecutionType.Server);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteDeleteSet", this, SingleplayerExecutionType.Server);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteSaveEdits", this, SingleplayerExecutionType.Server);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "RemoteQuickDeleteObject", this, SingleplayerExecutionType.Server);
      GetRPCManager().AddRPC("RPC_BuildingSetManager", "ExportSetToCode", this, SingleplayerExecutionType.Server);
      //-------
   }

   override void OnInit()
   {
		Load();
   }
	
	// Loads the sets listed in the journal plus any set file the folder scan finds (CF.FindFileEx; server only). Old
	// binary .vpp sets are converted to JSON on the way (VPPBuildingSetStore), JSON sets of an older format version
	// are upgraded and saved again. A listed file that is gone or unreadable is dropped from the journal and logged
	// (the file itself is never touched).
	override void Load()
	{
		if (!GetGame().IsServer())
		{
			return;
		}

		if (!FileExist(JSONPATH))
		{
			MakeDirectory(JSONPATH);
		}

		bool hadJournal = m_Journal.Load();
		int scanned = m_Journal.ScanForSets();
		int loaded = 0;
		int dropped = 0;
		m_ConvertedCount = 0;
		m_UpgradedCount = 0;
		array<string> paths = new array<string>();
		m_Journal.GetPaths(paths);
		foreach (string path : paths)
		{
			string loadedPath = path;
			BuildingSet bSet = null;
			if (VPPBuildingSetStore.IsLegacyPath(path))
			{
				loadedPath = ConvertLegacySet(path);
				if (loadedPath == "")
				{
					continue;
				}

				bSet = m_ConvertedSet;
				m_ConvertedSet = null;
			}
			else
			{
				bSet = ReadJsonSet(path);
			}

			if (!bSet)
			{
				string reason = "unreadable";
				if (!FileExist(path))
				{
					reason = "missing";
				}

				m_Journal.Remove(path, reason);
				GetSimpleLogger().Log("[BuildingSetManager] Building set file " + reason + ", removed from the journal: " + path);
				dropped++;
				continue;
			}

			string setName = bSet.GetName();
			if (GetBuildingSetByName(setName))
			{
				GetSimpleLogger().Log("[BuildingSetManager] Skipped " + loadedPath + ": a set named " + setName + " is already loaded");
				continue;
			}

			m_BuildingSets.Insert(bSet);
			m_Journal.Put(setName, loadedPath, "");
			loaded++;
		}

		if (!hadJournal || m_Journal.IsDirty())
		{
			m_Journal.Save();
		}

		string summary = "[BuildingSetManager] Journal: " + loaded.ToString() + " set(s) loaded, " + scanned.ToString() + " found by the folder scan, " + dropped.ToString() + " dropped";
		int currentVersion = VPPBuildingSetFile.CURRENT_VERSION;
		string formatText = ", " + m_ConvertedCount.ToString() + " converted from .vpp, " + m_UpgradedCount.ToString() + " upgraded to format " + currentVersion.ToString();
		summary = summary + formatText;
		GetSimpleLogger().Log(summary);
		Print(summary);
		SpawnActiveBuildings();
	}

	// A JSON set (null when it cannot be read). A file of an older format version is saved again right away.
	protected BuildingSet ReadJsonSet(string path)
	{
		VPPBuildingSetReadInfo info = new VPPBuildingSetReadInfo();
		BuildingSet bSet = VPPBuildingSetStore.ReadJson(path, info);
		if (!bSet)
		{
			if (info.Error != "")
			{
				GetSimpleLogger().Log("[BuildingSetManager] " + info.Error);
			}

			return null;
		}

		// a file of a NEWER version is never rewritten automatically (its unknown fields would be lost)
		if (info.Upgraded && info.FileVersion <= VPPBuildingSetFile.CURRENT_VERSION)
		{
			if (VPPBuildingSetStore.Write(bSet, path))
			{
				int currentVersion = VPPBuildingSetFile.CURRENT_VERSION;
				string fromText = info.FileVersion.ToString();
				string toText = currentVersion.ToString();
				GetSimpleLogger().Log("[BuildingSetManager] Upgraded " + path + " from format " + fromText + " to " + toText + " (previous file kept as .bak)");
				m_UpgradedCount++;
			}
		}

		return bSet;
	}

	// An old binary set: written as JSON at the set's normal path, then the .vpp is moved to the backups. Returns the
	// path the set is loaded from (m_ConvertedSet holds it), or "" when the .vpp only had to be moved away / the set
	// is loaded through another journal entry. A .vpp that cannot be read leaves m_ConvertedSet null (dropped, kept).
	protected string ConvertLegacySet(string path)
	{
		m_ConvertedSet = VPPBuildingSetStore.ReadLegacy(path);
		if (!m_ConvertedSet)
		{
			return path;
		}

		string setName = m_ConvertedSet.GetName();
		string jsonPath = m_Journal.PathFor(setName);
		if (FileExist(jsonPath))
		{
			// converted before (or saved since): the JSON wins, the leftover .vpp is moved away
			m_ConvertedSet = null;
			m_Journal.Remove(path, "superseded");
			VPPBuildingSetStore.ArchiveLegacy(path);
			if (m_Journal.HasPath(jsonPath))
			{
				return "";
			}

			m_ConvertedSet = ReadJsonSet(jsonPath);
			return jsonPath;
		}

		if (!VPPBuildingSetStore.Write(m_ConvertedSet, jsonPath))
		{
			GetSimpleLogger().Log("[BuildingSetManager] Could not convert " + path + " to JSON; it is loaded from the old file this time and converted at the next start");
			return path;
		}

		m_Journal.Remove(path, "converted");
		m_Journal.Put(setName, jsonPath, "convert");
		VPPBuildingSetStore.ArchiveLegacy(path);
		GetSimpleLogger().Log("[BuildingSetManager] Converted " + path + " to " + jsonPath);
		m_ConvertedCount++;
		return jsonPath;
	}

	void SaveBuildingSet(BuildingSet bSet)
	{
		if (!bSet)
		{
			return;
		}

		string setName = bSet.GetName();
		string path = m_Journal.PathFor(setName);
		if (VPPBuildingSetStore.Write(bSet, path))
		{
			m_Journal.Put(setName, path, "save");
			m_Journal.SaveIfDirty();
			GetSimpleLogger().Log("[BuildingSetManager] Saved: " + path);
			return;
		}
		GetSimpleLogger().Log("[BuildingSetManager] FAILED To Save: "+path);
	}

	// The old .vpp of a set name, if one is still around (a conversion that failed): removed with the set.
	protected void DeleteLegacyFile(string setName)
	{
		string legacyPath = m_Journal.LegacyPathFor(setName);
		m_Journal.Remove(legacyPath, "delete");
		if (FileExist(legacyPath))
		{
			DeleteFile(legacyPath);
		}
	}
	
	// The set names, to the Object Manager.
	protected void SendSetList(PlayerIdentity sender)
	{
		GetRPCManager().VSendRPC("RPC_MenuObjectManager", "HandleData", new Param1<ref array<string>>(GetSetsNames()), true, sender);
	}

	//use only when you edit an exisitng building set name (to delete old set file)
	void DeleteSavedFile(string fileName)
	{
		string path = m_Journal.PathFor(fileName);
		m_Journal.Remove(path, "rename");
		DeleteLegacyFile(fileName);
		m_Journal.SaveIfDirty();
		if (FileExist(path))
		{
			//the caller saves the set again right after (same or new name); the last copy is kept as .bak meanwhile
			string backupPath = path + ".bak";
			if (FileExist(backupPath))
			{
				DeleteFile(backupPath);
			}

			CopyFile(path, backupPath);
			DeleteFile(path);
			GetSimpleLogger().Log("[BuildingSetManager] Deleted file: "+ fileName);
			return;
		}
		GetSimpleLogger().Log("[BuildingSetManager] File not found: "+ fileName);
	}
	
	array<string> GetSetsNames()
	{
		array<string> setNames = new array<string>;
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
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager", "", false)) return;

			SendSetList(sender);
		}
	}
	
	void GetSetData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager", "", false)) return;
			
			BuildingSet buildSet = GetBuildingSetByName(data.param1);
			if (buildSet == null){
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Error, Building Set: "+data.param1+"\nwas not found!",NotifyTypes.ERROR);
				return;
			}
			
			GetRPCManager().VSendRPC("RPC_MenuObjectManager","HandleSetsData", new Param1<ref BuildingSet>(buildSet),true,sender);
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
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) created new building set (%3)", sender.GetName(), sender.GetPlainId(), data.param1));
			AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] RemoteAddNewSet() Added new buildingset: " + data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, rpt);
			//Send new data to client
			SendSetList(sender);
		}
	}
	
	void RemoteUpdateSet(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<string,string,bool> data;
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager:UpdateSet")) return;
			
			BuildingSet toEdit = GetBuildingSetByName(data.param1);
			if (toEdit != null)
			{
				toEdit.SetActive(data.param3);
				toEdit.SetName(data.param2);
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Updated Building set: "+data.param1 + " to new name\n"+data.param2,NotifyTypes.NOTIFY);
				GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) updated building set (%3)", sender.GetName(), sender.GetPlainId(), data.param1));
				DeleteSavedFile(data.param1); //Delete old sets save file (nothing to do with memeory as this file is not loaded)
				SaveBuildingSet(toEdit);
				//Send new data to client
				SendSetList(sender);
				AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] RemoteUpdateSet() Updated buildingset: " + data.param1);
				GetWebHooksManager().PostData(AdminActivityMessage, rpt);
				return;
			}
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_ERROR_UPDATE_BUILDINGSET",NotifyTypes.NOTIFY);
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
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) deleted building set (%3)", sender.GetName(), sender.GetPlainId(), data.param1));
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Deleted building set: "+data.param1,NotifyTypes.NOTIFY);
			//Send new data to client
			SendSetList(sender);
			AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] RemoteDeleteSet() Deleted buildingset: " + data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, rpt);
			return;
		}
		GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_ERROR_DELETE_BUILDINGSET",NotifyTypes.NOTIFY);
	}
	
	void RemoteSaveEdits(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<ref array<ref SpawnedBuilding>,string> data;
		if (!ctx.Read(data)) return;

		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager:EditSet")) return;

			BuildingSet bSet = GetBuildingSetByName(data.param2);
			if (bSet == null)
			{
				GetSimpleLogger().Log("[BuildingSetManager] Error saving building set, not found: " + data.param2);
				return;
			}

			bool wasActive = bSet.GetActive();
			bSet.SetActive(false);
			bSet.ClearBuildings();
			bSet.SetBuildingsArray(data.param1);
			SaveBuildingSet(bSet);
			bSet.SetActive(wasActive);
			
			//Send new data to client
			SendSetList(sender);
			GetSimpleLogger().Log("[BuildingSetManager] Saving & Reloading Building Set:" + bSet.GetName());
			AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] RemoteSaveEdits() Saving & Reloading Building Set:" + bSet.GetName());
			GetWebHooksManager().PostData(AdminActivityMessage, rpt);
			return;
		}
	}
	
	void RemoteQuickDeleteObject(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<Object> data;
		if (!ctx.Read(data)) return;
		
		if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuObjectManager", "", false)) return;
		
		if (data.param1 != null)
			GetGame().ObjectDelete(data.param1);
	}
	
	void ExportSetToCode(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		BuildingSet bset = GetBuildingSetByName(data.param1);
		if (bset != null)
		{
			bset.ExportBuildings();
			AdminActivityMessage rpt = new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[BuildingSetManager] ExportSetToCode() Exported Building Set:" + data.param1);
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
         if (!bSet.GetActive())
         {
            ClearNetworkIds(bSet);
         }

         if(bSet.GetActive())
         {
            array<ref SpawnedBuilding> buildings;
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

	// A set that is not spawned keeps no network ids: the ids saved in its file belong to a past session (another
	// object may carry them now).
	protected void ClearNetworkIds(BuildingSet bSet)
	{
		array<ref SpawnedBuilding> buildings = bSet.GetBuildings();
		if (!buildings)
		{
			return;
		}

		foreach (SpawnedBuilding building : buildings)
		{
			if (building && !building.GetObject())
			{
				building.ClearNetworkId();
			}
		}
	}

   void AddBuildingSet(string name, bool active = false)
   {
      BuildingSet bSet = new BuildingSet(name, active);
      m_BuildingSets.Insert(bSet);
		SaveBuildingSet(bSet);
   }

	void AddBuildingToSet(string bSetName, string building, vector pos, vector orientation, bool active = false)
	{
		BuildingSet bSet = GetBuildingSetByName(bSetName);

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
      BuildingSet bSet = GetBuildingSetByName(bSetName);

      if(bSet)
      {
			string path = m_Journal.PathFor(bSet.GetName());
			DeleteFile(path);
			m_Journal.Remove(path, "delete");
			DeleteLegacyFile(bSet.GetName());
			m_Journal.SaveIfDirty();
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
      BuildingSet building = GetBuildingSetByName(bSetName);

      if(building)
      {
         building.DeleteBuildingByName(buildingName);
      }
		
		SaveBuildingSet(building);
   }

   void ToggleBuildingByName(string bSetName, string buildingName, bool active = false)
   {
      BuildingSet building = GetBuildingSetByName(bSetName);

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