/*
	XML Editor chip grid (XMLEditorChip.layout prefabs inside a WrapSpacerWidget).

	Modes: radio (exactly one chip ON, used for the category), multi (ON/OFF toggles) and tri-state
	(OFF = keep, ADD, REMOVE; used by the bulk editor). Chips are sized to their text:
	width = Math.Clamp(20 + 7 * label.LengthUtf8(), 56, 220) px (characters, not bytes). MIXED marks an inherited state
	(shown, not set in the edited definition). m_OnChanged is invoked with (int idx, int state).
*/
class VPPXEChipGroup : Managed
{
	const static int CHIP_OFF = 0;
	const static int CHIP_ON = 1;
	const static int CHIP_MIXED = 2;
	const static int CHIP_ADD = 3;
	const static int CHIP_REMOVE = 4;

	protected Widget m_Spacer;
	protected bool m_Radio;
	protected bool m_TriState;
	protected bool m_Enabled;
	protected ref array<Widget> m_Roots;
	protected ref array<Widget> m_Fills;
	protected ref array<TextWidget> m_Texts;
	protected ref array<int> m_States;
	protected ref array<string> m_Labels;
	ref ScriptInvoker m_OnChanged;

	void VPPXEChipGroup(Widget wrapSpacer, bool radio, bool triState)
	{
		m_Spacer = wrapSpacer;
		m_Radio = radio;
		m_TriState = triState;
		m_Enabled = true;
		m_Roots = new array<Widget>();
		m_Fills = new array<Widget>();
		m_Texts = new array<TextWidget>();
		m_States = new array<int>();
		m_Labels = new array<string>();
		m_OnChanged = new ScriptInvoker();
	}

	void ~VPPXEChipGroup()
	{
		Clear();
	}

	Widget GetSpacer()
	{
		return m_Spacer;
	}

	int Count()
	{
		return m_States.Count();
	}

	string GetLabel(int idx)
	{
		if (idx < 0 || idx >= m_Labels.Count())
		{
			return "";
		}

		return m_Labels[idx];
	}

	void Clear()
	{
		WidgetEventHandler handler = WidgetEventHandler.GetInstance();
		for (int i = 0; i < m_Roots.Count(); i++)
		{
			Widget chipRoot = m_Roots[i];
			if (!chipRoot)
			{
				continue;
			}

			if (handler)
			{
				handler.UnregisterWidget(chipRoot);
			}

			chipRoot.Unlink();
		}

		m_Roots.Clear();
		m_Fills.Clear();
		m_Texts.Clear();
		m_States.Clear();
		m_Labels.Clear();
	}

	void SetItems(array<string> labels)
	{
		Clear();
		if (!m_Spacer || !labels)
		{
			return;
		}

		WidgetEventHandler handler = WidgetEventHandler.GetInstance();
		for (int i = 0; i < labels.Count(); i++)
		{
			string label = labels[i];
			Widget chipRoot = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorChip, m_Spacer);
			Widget chipFill = null;
			TextWidget chipText = null;
			if (chipRoot)
			{
				chipFill = chipRoot.FindAnyWidget("ChipFill");
				chipText = TextWidget.Cast(chipRoot.FindAnyWidget("ChipText"));
				if (chipText)
				{
					chipText.SetText(label);
				}

				int width = Math.Clamp(20 + 7 * label.LengthUtf8(), 56, 220);
				chipRoot.SetSize(width, 24);
				if (handler)
				{
					handler.RegisterOnMouseButtonDown(chipRoot, this, "OnChipDown");
				}
			}

			m_Roots.Insert(chipRoot);
			m_Fills.Insert(chipFill);
			m_Texts.Insert(chipText);
			m_States.Insert(CHIP_OFF);
			m_Labels.Insert(label);
			ApplyTint(m_States.Count() - 1);
		}

		m_Spacer.Update();
	}

	void Enable(bool state)
	{
		m_Enabled = state;
		for (int i = 0; i < m_Roots.Count(); i++)
		{
			Widget chipRoot = m_Roots[i];
			if (chipRoot)
			{
				chipRoot.Enable(state);
			}

			ApplyTint(i);
		}
	}

	bool IsEnabled()
	{
		return m_Enabled;
	}

	void SetState(int idx, int state)
	{
		if (idx < 0 || idx >= m_States.Count())
		{
			return;
		}

		m_States[idx] = state;
		ApplyTint(idx);
	}

	int GetState(int idx)
	{
		if (idx < 0 || idx >= m_States.Count())
		{
			return CHIP_OFF;
		}

		return m_States[idx];
	}

	void SetAll(int state)
	{
		for (int i = 0; i < m_States.Count(); i++)
		{
			m_States[i] = state;
			ApplyTint(i);
		}
	}

	//chips [0, plainCount) map to bits of plain, chips [plainCount, Count()) to bits of group; set bits get onState
	void SetMask(int plain, int group, int plainCount, int onState = 1)
	{
		for (int i = 0; i < m_States.Count(); i++)
		{
			bool isOn = false;
			if (i < plainCount)
			{
				if (i < 32 && (plain & (1 << i)) != 0)
				{
					isOn = true;
				}
			}
			else
			{
				int groupIdx = i - plainCount;
				if (groupIdx < 32 && (group & (1 << groupIdx)) != 0)
				{
					isOn = true;
				}
			}

			if (isOn)
			{
				m_States[i] = onState;
			}
			else
			{
				m_States[i] = CHIP_OFF;
			}

			ApplyTint(i);
		}
	}

	//bits of the chips [0, plainCount) whose state is not OFF
	int GetMask(int plainCount)
	{
		int mask = 0;
		for (int i = 0; i < plainCount && i < m_States.Count() && i < 32; i++)
		{
			if (m_States[i] != CHIP_OFF)
			{
				mask = mask | (1 << i);
			}
		}

		return mask;
	}

	//bits of the chips [plainCount, Count()) whose state is not OFF
	int GetGroupMask(int plainCount)
	{
		int mask = 0;
		for (int i = plainCount; i < m_States.Count(); i++)
		{
			int groupIdx = i - plainCount;
			if (groupIdx < 32 && m_States[i] != CHIP_OFF)
			{
				mask = mask | (1 << groupIdx);
			}
		}

		return mask;
	}

	void OnChipDown(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT || !m_Enabled)
		{
			return;
		}

		int idx = m_Roots.Find(w);
		if (idx < 0)
		{
			return;
		}

		int state = m_States[idx];
		if (m_Radio)
		{
			if (state == CHIP_ON)
			{
				return;
			}

			for (int i = 0; i < m_States.Count(); i++)
			{
				m_States[i] = CHIP_OFF;
				ApplyTint(i);
			}

			m_States[idx] = CHIP_ON;
		}
		else if (m_TriState)
		{
			if (state == CHIP_OFF)
			{
				m_States[idx] = CHIP_ADD;
			}
			else if (state == CHIP_ADD)
			{
				m_States[idx] = CHIP_REMOVE;
			}
			else
			{
				m_States[idx] = CHIP_OFF;
			}
		}
		else
		{
			if (state == CHIP_OFF)
			{
				m_States[idx] = CHIP_ON;
			}
			else
			{
				m_States[idx] = CHIP_OFF;
			}
		}

		ApplyTint(idx);
		m_OnChanged.Invoke(idx, m_States[idx]);
	}

	protected void ApplyTint(int idx)
	{
		if (idx < 0 || idx >= m_States.Count())
		{
			return;
		}

		int state = m_States[idx];
		int fillColor = ARGB(255, 27, 30, 34);
		int textColor = ARGB(255, 154, 160, 166);
		if (state == CHIP_ON)
		{
			fillColor = ARGB(255, 232, 163, 61);
			textColor = ARGB(255, 255, 255, 255);
		}
		else if (state == CHIP_MIXED)
		{
			fillColor = ARGB(150, 61, 125, 214);
			textColor = ARGB(255, 255, 255, 255);
		}
		else if (state == CHIP_ADD)
		{
			fillColor = ARGB(255, 76, 175, 80);
			textColor = ARGB(255, 255, 255, 255);
		}
		else if (state == CHIP_REMOVE)
		{
			fillColor = ARGB(255, 194, 69, 69);
			textColor = ARGB(255, 255, 255, 255);
		}

		if (!m_Enabled)
		{
			textColor = ARGB(255, 92, 97, 102);
		}

		Widget chipFill = m_Fills[idx];
		if (chipFill)
		{
			chipFill.SetColor(fillColor);
		}

		TextWidget chipText = m_Texts[idx];
		if (chipText)
		{
			chipText.SetColor(textColor);
		}
	}
};
