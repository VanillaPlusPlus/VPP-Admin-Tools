/*
	Admin scale (context menu "Set scale") for animals. Same sync variable as modded BuildingBase, but the
	animation can rewrite the transform every frame, so a scaled animal re-applies its scale in EOnPostFrame
	(the SurvivorBase m_VPlayerScale precedent), on the server and on every client.
	0 = never scaled by an admin. Infected: see modded ZombieBase.
*/
modded class AnimalBase
{
	protected float m_VPPEntityScale;

	void AnimalBase()
	{
		RegisterNetSyncVariableFloat("m_VPPEntityScale", 0.0, 100.0, 3);
	}

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

	//the event mask is never cleared: another mod may use POSTFRAME on the same animal
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
