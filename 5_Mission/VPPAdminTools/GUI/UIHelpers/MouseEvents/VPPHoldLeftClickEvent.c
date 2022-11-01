class VPPHoldLeftClickEvent: VPPMouseEventBase
{
	VPPMarkerObj marker; //marker object

	void ~VPPHoldLeftClickEvent()
	{
		if (marker)
			GetGame().ObjectDelete(marker);
	}

	override bool GetTriggerCondition()
	{
		return (GetGame().GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) && g_Game.IsLeftCtrlDown();
	}

	override void OnCondition()
	{
		if (!marker)
		{
			vector pos = GetGame().GetPlayer().GetPosition();
			pos[1] = GetGame().SurfaceY(pos[0], pos[1]);
			marker = VPPMarkerObj.Cast(GetGame().CreateObjectEx("VPPMarkerObj", pos, ECE_LOCAL));
		}

		if (marker)
		{
			marker.TransformUpdate();
		}
		
		super.OnCondition();
	}

	override bool OnConditionEnd()
	{
		if (m_fnStop == string.Empty || !m_fnStop)
			return false;

		if (!marker)
			return false;

		GetGame().GameScript.CallFunctionParams(m_cbInstance, m_fnStop, null, null);
		GetGame().ObjectDelete(marker);
		return true;
	}

	VPPMarkerObj GetMarker()
	{
		return marker;
	}
};