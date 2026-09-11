modded class ConstructionActionData
{
	ref array<ConstructionPart> 	m_AdminBuildParts;
	protected ActionVariantManager	m_AdminActionVariantManager;

	void ConstructionActionData()
	{
		m_AdminBuildParts = new array<ConstructionPart>;

		if ( GetGame().IsClient() || !GetGame().IsMultiplayer() )
		{
			m_AdminActionVariantManager = ActionManagerClient.GetVariantManager(ActionAdminBaseBuilder);
			m_AdminActionVariantManager.GetOnUpdateInvoker().Insert(VPPAT_OnUpdateActions);
		}
	}

	//client side
	void VPPAT_OnUpdateActions( Object item, Object target, int component_index )
	{
		BaseBuildingBase base_building_object = BaseBuildingBase.Cast(target);
		if (base_building_object)
		{
			string main_part_name = target.GetActionComponentName(component_index);
			base_building_object.GetConstruction().VPPAT_GetConstructionPartsToBuild(main_part_name, m_AdminBuildParts, null);
			m_AdminActionVariantManager.SetActionVariantCount(m_AdminBuildParts.Count());
		}
		else
		{
			m_AdminBuildParts.Clear();
			m_AdminActionVariantManager.Clear();
		}
	}

	ConstructionPart VPPAT_GetBuildPartAtIndex(int idx)
	{
		if( m_AdminBuildParts.Count() > idx )
		{
			return m_AdminBuildParts.Get( idx );
		}
		return null;
	}
};