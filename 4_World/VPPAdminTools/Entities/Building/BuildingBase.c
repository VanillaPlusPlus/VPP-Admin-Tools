/*
	Admin scale (context menu "Set scale") for buildings: mission / building-set placed ones and static map buildings.
	The engine does not replicate SetScale after creation, so the value travels as a sync variable and every
	client re-applies it (late joiners and bubble re-entry get it with the object).
	Static map objects are not replicated by default: they are registered with RegisterNetworkStaticObject first
	(the vanilla Land_Underground_Panel.Interact order), after which their sync variables reach the clients.
	0 = never scaled by an admin: the object keeps whatever scale it spawned with.
	Building itself is not moddable; BuildingBase covers House and every Land_* building.
*/
modded class BuildingBase
{
	protected float m_VPPEntityScale;

	void BuildingBase()
	{
		RegisterNetSyncVariableFloat("m_VPPEntityScale", 0.0, 100.0, 3);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		VPPApplyEntityScale();
	}

	//server
	void VPPSetEntityScale(float scale)
	{
		if (!GetGame().IsServer())
		{
			return;
		}

		//static map ids have a negative high word; false = already registered
		int low;
		int high;
		GetNetworkID(low, high);
		if (high < 0)
		{
			GetGame().RegisterNetworkStaticObject(this);
		}

		m_VPPEntityScale = scale;
		VPPApplyEntityScale();
		SetSynchDirty();
	}

	float VPPGetEntityScale()
	{
		return m_VPPEntityScale;
	}

	protected void VPPApplyEntityScale()
	{
		if (m_VPPEntityScale <= 0)
		{
			return;
		}

		if (Math.AbsFloat(GetScale() - m_VPPEntityScale) < 0.0005)
		{
			return;
		}

		SetScale(m_VPPEntityScale);
		Update();
	}
};
