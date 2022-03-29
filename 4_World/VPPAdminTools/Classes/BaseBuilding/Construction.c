modded class Construction
{
	bool VPPAT_CanBuildPart(string part_name, ItemBase tool)
	{
		if (!IsPartConstructed(part_name) && HasRequiredPart(part_name) && !HasConflictPart(part_name))
		{
			return true;
		}
		return false;
	}

	//parent
	BaseBuildingBase VPPAT_GetParent()
	{
		return m_Parent;
	}

	//Get all construction parts that can be build (at that current time)
	void VPPAT_GetConstructionPartsToBuild(string main_part_name, out array<ConstructionPart> construction_parts, ItemBase tool, out string real_constructionTarget)
	{
		construction_parts.Clear();
		string part_name;
		ConstructionPart value;
		string key;
		
		for ( int i = 0; i < m_ConstructionParts.Count(); ++i )
		{
			key = m_ConstructionParts.GetKey(i);
			value = m_ConstructionParts.Get(key);
		
			if ( main_part_name == value.GetMainPartName() && VPPAT_CanBuildPart(value.GetPartName(), tool))
			{
				construction_parts.Insert(value);
			}
			
			if ( main_part_name == value.GetPartName() )
			{
				part_name = value.GetMainPartName();
			}
		}
		
		if( construction_parts.Count() == 0 && part_name )
		{
			for ( i = 0; i < m_ConstructionParts.Count(); ++i )
			{
				key = m_ConstructionParts.GetKey(i);
				value = m_ConstructionParts.Get(key);
		
				if ( part_name == value.GetMainPartName() && VPPAT_CanBuildPart(value.GetPartName(), tool))
				{
					construction_parts.Insert(value);
				}
			}
		}
	}
};