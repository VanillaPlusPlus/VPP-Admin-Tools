/*
	Admin scale (context menu "Set scale") for infected, same as modded AnimalBase.
	0 = never scaled by an admin.
*/
modded class ZombieBase
{
	protected float m_VPPEntityScale;

	void ZombieBase()
	{
		RegisterNetSyncVariableFloat("m_VPPEntityScale", 0.0, 100.0, 3);
	}

	//vanilla ZombieBase.OnVariablesSynchronized skips super, so this is the only hook for infected
	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		VPPOnEntityScaleSynced();
	}

	override void EOnPostFrame(IEntity other, int extra)
	{
		super.EOnPostFrame(other, extra);
		VPPApplyEntityScale();
	}

	//server
	void VPPSetEntityScale(float scale)
	{
		if (!GetGame().IsServer())
		{
			return;
		}

		m_VPPEntityScale = scale;
		VPPOnEntityScaleSynced();
		SetSynchDirty();
	}

	float VPPGetEntityScale()
	{
		return m_VPPEntityScale;
	}

	//the event mask is never cleared: vanilla sets INIT | CONTACT and another mod may use POSTFRAME
	protected void VPPOnEntityScaleSynced()
	{
		if (m_VPPEntityScale <= 0)
		{
			return;
		}

		SetEventMask(EntityEvent.POSTFRAME);
		VPPApplyEntityScale();
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
	}
};
