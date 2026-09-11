class VPPFilterEntry : VPPPlayerTemplate
{
    private CheckBoxWidget  m_FilterToggle;
    private ButtonWidget    m_btnDeleteFilter;
    private ButtonWidget    m_btnColor;
    private Widget          m_ColorSwatch;
	private bool   		    m_IsVisible;
	GridSpacerWidget 	    m_Grid;
	ref EspFilterProperties m_Props;

	//number of selectable colors (index order mirrors ColorFromIndex / ColorNameFromIndex)
	const static int COLOR_COUNT = 8;

    void VPPFilterEntry(GridSpacerWidget grid, EspFilterProperties props, bool startCheck = false)
    {
		m_Grid       = grid;
        m_LayoutPath = VPPATUIConstants.FilterEntry;
        m_EntryBox = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
        m_FilterToggle    = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("CheckBox"));
        m_btnDeleteFilter = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnDeleteFilter"));
        m_btnColor        = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnColor"));
        m_ColorSwatch     = m_EntryBox.FindAnyWidget("colorSwatch");
        m_FilterToggle.SetText(props.filterType);
        m_FilterToggle.SetChecked(startCheck);
		m_IsVisible = true;
		m_Props = props;
		m_EntryBox.SetHandler(this);

		//show the current color on the row's swatch button
		SetColorDisplay(props.comboIndex);
    }

    void ~VPPFilterEntry()
    {
        if (m_EntryBox != null)
        m_EntryBox.Unlink();
    }

	override bool OnClick(Widget w, int x, int y, int button)
	{
		EspToolsMenu espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));

		if (w == m_FilterToggle)
		{
			if (espMenu)
				espMenu.ClearTrackers();
			return true;
		}

		if (w == m_btnColor)
		{
			//toggle the shared color picker (hosted on Main, draws on top of the scroll list)
			if (espMenu)
				espMenu.ToggleColorPicker(this, m_btnColor);
			return true;
		}

		if (w == m_btnDeleteFilter)
		{
			if (espMenu)
				espMenu.RemoveFilterProps(m_Props);
			return true;
		}

		return false;
	}

	//apply a chosen color index to this filter (called by EspToolsMenu when a swatch is picked)
	void ApplyColorIndex(int index)
	{
		m_Props.color      = ColorFromIndex(index);
		m_Props.comboIndex = index;
		SetColorDisplay(index);
	}

	//tint the row swatch to reflect the current color
	void SetColorDisplay(int index)
	{
		if (m_ColorSwatch)
			m_ColorSwatch.SetColor(ColorFromIndex(index));
	}

	static int ColorFromIndex(int index)
	{
		switch (index)
		{
			case 0: return ARGB(255,255,255,255); //White
			case 1: return ARGB(255,0,255,0);     //Green
			case 2: return ARGB(255,0,0,255);     //Blue
			case 3: return ARGB(255,255,255,0);   //Yellow
			case 4: return ARGB(255,226,109,18);  //Orange
			case 5: return ARGB(255,255,0,0);     //Red
			case 6: return ARGB(255,100,11,234);  //Purple
			case 7: return ARGB(255,0,0,0);       //Black
		}
		return ARGB(255,255,255,255);
	}

	void SetVisible(bool state)
	{
		m_IsVisible = state;
		m_EntryBox.Show(state);
	}

	bool IsVisible()
	{
		return m_EntryBox.IsVisible();
	}

    CheckBoxWidget GetCheckWidget()
    {
        return m_FilterToggle;
    }

    string GetFilterName()
    {
        return m_Props.filterType;
    }

	int GetFilterColor()
	{
		return m_Props.color;
	}

	void SetSelected(bool checked)
	{
		m_FilterToggle.SetChecked(checked);
	}

    bool IsSelected()
    {
        return m_FilterToggle.IsChecked();
    }
}
