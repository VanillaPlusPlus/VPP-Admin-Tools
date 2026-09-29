/*
	On/off chip of the webhooks menu: a VPPButton "Btn<Name>" with a tinted "Fill<Name>" panel and a "Txt<Name>" label
	(MenuWebHooks.layout toggle()). The owner routes clicks: if (toggle.Is(w)) toggle.Flip().
*/
class VPPWebhookToggle : Managed
{
	protected ButtonWidget m_Button;
	protected Widget m_Fill;
	protected TextWidget m_Text;
	protected bool m_On;
	protected bool m_Enabled;

	void VPPWebhookToggle(Widget root, string buttonName)
	{
		string suffix = buttonName.Substring(3, buttonName.Length() - 3);
		m_Button = ButtonWidget.Cast(root.FindAnyWidget(buttonName));
		m_Fill = root.FindAnyWidget("Fill" + suffix);
		m_Text = TextWidget.Cast(root.FindAnyWidget("Txt" + suffix));
		m_On = false;
		m_Enabled = true;
		Paint();
	}

	bool Is(Widget w)
	{
		return w != null && w == m_Button;
	}

	bool IsOn()
	{
		return m_On;
	}

	void SetOn(bool on)
	{
		m_On = on;
		Paint();
	}

	void Flip()
	{
		m_On = !m_On;
		Paint();
	}

	void SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
		if (m_Button)
		{
			m_Button.Enable(enabled);
		}

		Paint();
	}

	void Show(bool visible)
	{
		if (m_Button)
		{
			m_Button.Show(visible);
		}
	}

	protected void Paint()
	{
		int fillColor = ARGB(255, 27, 30, 34);
		int textColor = VPPWebhookUi.CLR_SECONDARY;
		if (m_On)
		{
			fillColor = ARGB(235, 232, 163, 61);
			textColor = ARGB(255, 20, 22, 24);
		}

		if (!m_Enabled)
		{
			fillColor = ARGB(120, 27, 30, 34);
			textColor = VPPWebhookUi.CLR_OFF;
			if (m_On)
			{
				fillColor = ARGB(110, 232, 163, 61);
			}
		}

		if (m_Fill)
		{
			m_Fill.SetColor(fillColor);
		}

		if (m_Text)
		{
			m_Text.SetColor(textColor);
		}
	}
};
