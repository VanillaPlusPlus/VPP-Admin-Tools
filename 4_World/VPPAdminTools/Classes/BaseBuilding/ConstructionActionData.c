modded class ConstructionActionData
{
	void VPPAT_RefreshPartsToBuild(string main_part_name, ItemBase tool)
	{
		BaseBuildingBase base_building_object = BaseBuildingBase.Cast(m_Target);
		if (base_building_object)
		{
			base_building_object.GetConstruction().VPPAT_GetConstructionPartsToBuild(main_part_name, m_BuildParts, tool, m_MainPartName);
		}
	}
};