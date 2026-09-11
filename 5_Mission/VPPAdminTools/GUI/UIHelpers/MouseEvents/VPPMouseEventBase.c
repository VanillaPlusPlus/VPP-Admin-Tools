class VPPMouseEventBase: ScriptedWidgetEventHandler
{
	protected Class  m_cbInstance;
	protected string m_fnName;	//method to call within CB instance each frame during condition-when-true
	protected string m_fnStop; 	//method to call within CB instance after condition end
	protected float  m_clickDealy; //seconds
	protected bool   m_active;
	protected bool	 m_processCb;
	protected float  m_TimePassed;
	protected bool   m_reset;

	void VPPMouseEventBase(Class cb, string fn, string fnStop, bool processCb = true, float delay = 0.3)
	{
		m_cbInstance  	= cb;
		m_fnName 		= fn;
		m_fnStop 		= fnStop;
		m_processCb		= processCb;
		m_clickDealy 	= delay;
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(this.OnUpdateLoop);
	}

	void ~VPPMouseEventBase()
	{
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(this.OnUpdateLoop);
	}

	void OnUpdateLoop(float tDelta)
	{
		if (!m_active)
			return;

		bool conditionState = GetTriggerCondition();
		if (conditionState)
			m_TimePassed += tDelta; //Apply delay

		if (m_TimePassed >= m_clickDealy && conditionState)
		{
			m_reset = false;
			OnCondition();
		}
		else if (!conditionState && !m_reset)
		{
			m_reset = true;
			m_TimePassed = 0.0;
			OnConditionEnd();
		}
	}

	//override this with logic you need to trigger
	//default is mouse left-click hold
	bool GetTriggerCondition()
	{
		return (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK);
	}

	//Called once after left condition no longer matches
	bool OnConditionEnd()
	{
		if (m_processCb)
		{
			GetGame().GameScript.CallFunctionParams(m_cbInstance, m_fnStop, null, null);
			return true;
		}
		return false;
	}

	//Called every update while condition matches
	void OnCondition()
	{
		if (!m_cbInstance)
		{
			Print("VPPMouseEvent::OnCondition() "+ m_cbInstance + " is null!");
			return;
		}

		if (m_processCb)
			GetGame().GameScript.CallFunctionParams(m_cbInstance, m_fnName, null, null);
	}

	void SetActive(bool active)
	{
		m_active = active;
	}

	bool IsActive()
	{
		return m_active;
	}
};