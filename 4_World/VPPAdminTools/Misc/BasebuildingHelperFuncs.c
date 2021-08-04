class BasebuildingHelperFuncs
{
	static const string bbExpansionTypes[4] = {"ExpansionSafeMini", "ExpansionSafeMedium", "ExpansionSafeLarge", "ExpansionWall6x3"};
	static const int BB_ITEMS_ARR = 4;

	static bool HasCombinationLock(Object obj, string type_name, out int combination = -1)
	{
		if (!obj)
			return false;
		if (type_name == string.Empty)
			return false;
		
		Print(obj);
		Print(type_name);
				
		switch(type_name)
		{
			//Vanilla
			case "BaseBuildingBase":
				Fence fence;
				if (Class.CastTo(fence, obj))
				{
					if (fence.GetCombinationLock())
					{
						combination = fence.GetCombinationLock().m_CombinationLocked;
						#ifdef VPPADMINTOOLS_DEBUG
						Print("[BasebuildingHelperFuncs] Combo for Fence: " + combination);
						#endif
						return true;
					}
				}
			break;
			
			//Expansion mod
			case "ExpansionBaseBuilding":
				GetRPCManager().VSendRPC("RPC_VPPESPTools", "RetriveCodeFromObj", new Param1<Object>(obj), true);
				return true;
			break;
			
			default:
				Print("[VPPADMINTOOLS] BasebuildingHelperFuncs :: Got unhandled typename -> " + type_name);
				return false;
			break;
		}
		return false;
	}
	
	static bool ClearComboFromLock(BaseBuildingBase obj)
	{
		if (!GetGame().IsServer())
			return false;
		
		//Vanilla
		Fence fence;
		if (Class.CastTo(fence, obj))
		{
			if (fence.GetCombinationLock())
			{
				GetGame().GameScript.CallFunction(fence, "SetBaseLockValues", NULL, NULL);
				fence.GetCombinationLock().UnlockServer(NULL, fence);
				#ifdef VPPADMINTOOLS_DEBUG
				Print("[BasebuildingHelperFuncs] Combo for Fence: " + fence);
				#endif
				return true;
			}
		}

		//DayZ Expansion
		string expansionMod = "ExpansionBaseBuilding";
		if (expansionMod.ToType())
		{
			ItemBase ib;
			if (Class.CastTo(ib, obj))
			{
				GetGame().GameScript.CallFunction(ib, "SetCode", NULL, "");
			}
			return true;
		}

		return false;
	}
	
	static bool IsRelatedToBaseBuilding(Object obj, out string type_name = "")
	{
		if (!obj)
			return false;
		
		string expansionMod = "ExpansionBaseBuilding"; //DayZ Expansion
		if (expansionMod.ToType())
		{
			for (int i = 0; i < BB_ITEMS_ARR; ++i)
			{
				//Check for specific type
				string obj_type = obj.GetType();
				typename type = obj_type.ToType();
				if (obj.IsKindOf(bbExpansionTypes[i]) || type.IsInherited(bbExpansionTypes[i].ToType()))
				{
					type_name = expansionMod;
					return true;
				}
			}
		}

		BaseBuildingBase baseObj; //Vanilla
		if (BaseBuildingBase.CastTo(baseObj,obj))
		{
			type_name = "BaseBuildingBase";
			return true;
		}
		return false;
	}

	//Expansion mod
	static bool IsItemStorageSafe(Object obj)
	{
		if (!obj)
			return false;

		string expansionMod = "ExpansionBaseBuilding";
		if (!expansionMod.ToType())
		{
			return false;
		}

		for (int i = 0; i < BB_ITEMS_ARR; ++i)
		{
			string obj_type = obj.GetType();
			typename type = obj_type.ToType();
			if (obj.IsKindOf(bbExpansionTypes[i]) || type.IsInherited(bbExpansionTypes[i].ToType()))
			{
				return true;
			}
		}
		return false;
	}
};