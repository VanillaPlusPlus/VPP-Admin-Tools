/*
	XML Editor numeric input (EditBox scriptclass "VPPXENumericEditHandler").

	Accepts an empty box, a lone minus while typing (only when the range allows negatives), or an
	optional leading minus followed by digits inside SetRange. Any other input restores the last
	valid text; it never resets the box to 0 (the shared EditBoxEventHandler rejects '-', so
	quantmin/quantmax -1 could not be typed). The mouse wheel steps by SetStep inside the range;
	over an empty box it does nothing and returns false so the scroll passes through.
	m_OnValueChanged is invoked with the EditBoxWidget whenever the accepted value changes by user input.
*/
class VPPXENumericEditHandler extends ScriptedWidgetEventHandler
{
	protected EditBoxWidget m_Box;
	protected string m_LastValid;
	protected int m_Step;
	protected int m_Min;
	protected int m_Max;
	protected bool m_Enabled;
	protected bool m_Suppress;
	ref ScriptInvoker m_OnValueChanged;

	void VPPXENumericEditHandler()
	{
		m_OnValueChanged = new ScriptInvoker();
		m_Step = 1;
		m_Min = 0;
		m_Max = 999999999;
		m_Enabled = true;
		m_Suppress = false;
		m_LastValid = "";
	}

	void OnWidgetScriptInit(Widget w)
	{
		m_Box = EditBoxWidget.Cast(w);
		if (!m_Box)
		{
			return;
		}

		m_Box.SetHandler(this);
		string current = m_Box.GetText();
		if (IsAcceptable(current))
		{
			m_LastValid = current;
		}
		else
		{
			m_LastValid = "";
			m_Suppress = true;
			m_Box.SetText("");
			m_Suppress = false;
		}
	}

	EditBoxWidget GetBox()
	{
		return m_Box;
	}

	void SetRange(int minValue, int maxValue)
	{
		m_Min = minValue;
		m_Max = maxValue;
	}

	void SetStep(int step)
	{
		if (step < 1)
		{
			step = 1;
		}

		m_Step = step;
	}

	void SetEnabled(bool state)
	{
		m_Enabled = state;
		if (m_Box)
		{
			m_Box.Enable(state);
		}
	}

	bool IsEnabled()
	{
		return m_Enabled;
	}

	void SetValue(int num)
	{
		if (!m_Box)
		{
			return;
		}

		string shown = num.ToString();
		m_Suppress = true;
		m_Box.SetText(shown);
		m_Suppress = false;
		m_LastValid = shown;
	}

	void SetEmpty()
	{
		if (!m_Box)
		{
			return;
		}

		m_Suppress = true;
		m_Box.SetText("");
		m_Suppress = false;
		m_LastValid = "";
	}

	bool HasValue()
	{
		string current = CurrentText();
		if (current == "" || current == "-")
		{
			return false;
		}

		return true;
	}

	int GetValue()
	{
		if (!HasValue())
		{
			return 0;
		}

		string current = CurrentText();
		return current.ToInt();
	}

	//the box text when it is acceptable, else the last accepted text
	protected string CurrentText()
	{
		if (!m_Box)
		{
			return m_LastValid;
		}

		string boxText = m_Box.GetText();
		if (IsAcceptable(boxText))
		{
			return boxText;
		}

		return m_LastValid;
	}

	protected bool IsAcceptable(string candidate)
	{
		int len = candidate.Length();
		if (len == 0)
		{
			return true;
		}

		int start = 0;
		string first = candidate.Get(0);
		if (first == "-")
		{
			if (m_Min >= 0)
			{
				return false;
			}

			start = 1;
		}

		if (len == start)
		{
			return true;
		}

		if (len - start > 9)
		{
			return false;
		}

		string digits = "0123456789";
		for (int i = start; i < len; i++)
		{
			string ch = candidate.Get(i);
			if (digits.IndexOf(ch) == -1)
			{
				return false;
			}
		}

		int num = candidate.ToInt();
		if (num < m_Min || num > m_Max)
		{
			return false;
		}

		return true;
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w != m_Box || m_Suppress)
		{
			return false;
		}

		string boxText = m_Box.GetText();
		if (!IsAcceptable(boxText))
		{
			m_Suppress = true;
			m_Box.SetText(m_LastValid);
			m_Suppress = false;
			return true;
		}

		if (boxText == m_LastValid)
		{
			return true;
		}

		m_LastValid = boxText;
		m_OnValueChanged.Invoke(m_Box);
		return true;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (w != m_Box || !m_Enabled)
		{
			return false;
		}

		//an empty box means "inherited"; let the scroll pass through instead of staging a 0
		if (!HasValue())
		{
			return false;
		}

		int num = GetValue();
		if (wheel > 0)
		{
			num = num + m_Step;
		}
		else
		{
			num = num - m_Step;
		}

		//integer clamp: Math.Clamp works in float and loses precision above 2^24
		if (num < m_Min)
		{
			num = m_Min;
		}
		else if (num > m_Max)
		{
			num = m_Max;
		}

		SetValue(num);
		m_OnValueChanged.Invoke(m_Box);
		return true;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w == m_Box)
		{
			SetFocus(null);
			return true;
		}

		return false;
	}
};
