class SliderEventHandler extends ScriptedWidgetEventHandler
{
	protected TextWidget 	   m_DisplayText;
	protected SliderWidget     m_root;
	protected float            m_step = 1.0; //How much to -- or ++ when scrolling

	void OnWidgetScriptInit(Widget w)
	{
		m_root = SliderWidget.Cast(w);
		m_root.SetHandler(this);
		m_DisplayText = TextWidget.Cast(m_root.GetChildren());
		
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.Update, 100, true);
	}
	
	void ~SliderEventHandler()
	{
	}

	void SetDisplayText(string text)
	{
		m_DisplayText.SetText(text);
	}

	void Update()
	{
		string sliderName = m_root.GetName();
		sliderName.Replace("Slider","");
		m_DisplayText.SetText(string.Format("%1:[%2]",sliderName,m_root.GetCurrent().ToString()));
	}
	
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		string sliderName = m_root.GetName();
		sliderName.Replace("Slider","");
		m_DisplayText.SetText(string.Format("%1:[%2]",sliderName,m_root.GetCurrent().ToString()));
		return true;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		float newValue;
		string sliderName = m_root.GetName();
		sliderName.Replace("Slider","");
		if (wheel <= -1){
			newValue = m_root.GetCurrent() - m_step;
			m_root.SetCurrent(newValue);
		}else{
			newValue = m_root.GetCurrent() + m_step;
			m_root.SetCurrent(newValue);
		}
		m_DisplayText.SetText(string.Format("%1:[%2]",sliderName,m_root.GetCurrent().ToString()));
		return true;
	}

	void SetStep(float step)
	{
		m_step = step;
	}

	float GetStep()
	{
		return m_step;
	}
};
/*
Make sure the slider you assign to this script class starts with Slider as its names in layouts eg: SliderHealth
*/