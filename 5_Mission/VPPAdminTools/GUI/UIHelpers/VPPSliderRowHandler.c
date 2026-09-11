/*
	Slider handler for vitals editor rows. Unlike SliderEventHandler (which writes
	"Name:[value]" into a child text), this displays the value-only readout in a
	sibling TextWidget named "<SliderName>Value" (e.g. SliderHealth -> SliderHealthValue).
*/
class VPPSliderRowHandler extends ScriptedWidgetEventHandler
{
	protected SliderWidget m_Root;
	protected TextWidget   m_Value;
	protected float        m_Step = 1.0;
	//display transform: shown value = current * scale + offset (e.g. heat buffer 0..60 -> -30..30)
	protected float        m_DisplayScale  = 1.0;
	protected float        m_DisplayOffset = 0.0;
	protected int          m_DisplayDecimals = 0;

	void OnWidgetScriptInit(Widget w)
	{
		m_Root = SliderWidget.Cast(w);
		m_Root.SetHandler(this);

		//periodic refresh picks up programmatic SetCurrent() calls (e.g. stats sync)
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.Refresh, 100, true);
	}

	void ~VPPSliderRowHandler()
	{
		if (GetGame() && GetGame().GetCallQueue(CALL_CATEGORY_GUI))
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.Refresh);
	}

	void SetDisplayTransform(float scale, float offset, int decimals = 0)
	{
		m_DisplayScale    = scale;
		m_DisplayOffset   = offset;
		m_DisplayDecimals = decimals;
		Refresh();
	}

	void Refresh()
	{
		if (!m_Root) return;

		//lazy lookup: sibling value widget may not exist yet during OnWidgetScriptInit
		if (!m_Value)
		{
			Widget parent = m_Root.GetParent();
			if (parent)
				m_Value = TextWidget.Cast(parent.FindAnyWidget(m_Root.GetName() + "Value"));
			if (!m_Value) return;
		}

		float shown = (m_Root.GetCurrent() * m_DisplayScale) + m_DisplayOffset;
		if (m_DisplayDecimals <= 0)
		{
			int rounded = Math.Round(shown);
			m_Value.SetText(rounded.ToString());
		}
		else
		{
			m_Value.SetText(shown.ToString());
		}
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		Refresh();
		return true;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (wheel <= -1)
			m_Root.SetCurrent(m_Root.GetCurrent() - m_Step);
		else
			m_Root.SetCurrent(m_Root.GetCurrent() + m_Step);
		Refresh();
		return true;
	}

	void SetStep(float step)
	{
		m_Step = step;
	}
};
