/*
	Single row in the PRESETS tab library list of the Item Manager.
	Clicking the row makes it the active preset; per-row Spawn / Delete buttons.
*/
class VPPPresetEntry
{
	private Widget            m_Root;
	private ButtonWidget      m_BtnSelect;
	private ImageWidget       m_Dot;
	private TextWidget        m_Name;
	private ButtonWidget      m_BtnSpawn;
	private ButtonWidget      m_BtnDelete;

	private MenuItemManager   m_Owner;
	private int               m_Index;

	void VPPPresetEntry(Widget parent, MenuItemManager owner, int index, string presetName, bool selected)
	{
		m_Owner = owner;
		m_Index = index;

		m_Root      = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPPresetEntry, parent);
		m_BtnSelect = ButtonWidget.Cast(m_Root.FindAnyWidget("PresetEntrySelect"));
		m_Dot       = ImageWidget.Cast(m_Root.FindAnyWidget("PresetEntryDot"));
		m_Name      = TextWidget.Cast(m_Root.FindAnyWidget("PresetEntryName"));
		m_BtnSpawn  = ButtonWidget.Cast(m_Root.FindAnyWidget("PresetEntrySpawn"));
		m_BtnDelete = ButtonWidget.Cast(m_Root.FindAnyWidget("PresetEntryDelete"));

		m_Name.SetText(presetName);
		SetSelected(selected);

		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_BtnSelect, this, "OnRowClick");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_BtnSpawn,  this, "OnRowClick");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_BtnDelete, this, "OnRowClick");
	}

	void ~VPPPresetEntry()
	{
		if (m_Root)
			m_Root.Unlink();
	}

	void SetSelected(bool selected)
	{
		if (selected)
		{
			m_Dot.SetColor(ARGB(255, 232, 163, 61));   //accent-orange: active preset
			m_Name.SetColor(ARGB(255, 232, 234, 237)); //text-primary
		}
		else
		{
			m_Dot.SetColor(ARGB(255, 92, 97, 102));    //text-muted
			m_Name.SetColor(ARGB(255, 154, 160, 166)); //text-secondary
		}
	}

	void OnRowClick(Widget w, int x, int y, int button)
	{
		if (!m_Owner) return;

		if (w == m_BtnSelect)
			m_Owner.OnPresetEntrySelect(m_Index);
		else if (w == m_BtnSpawn)
			m_Owner.OnPresetEntrySpawn(m_Index);
		else if (w == m_BtnDelete)
			m_Owner.OnPresetEntryDelete(m_Index);
	}

	int GetIndex()
	{
		return m_Index;
	}
}
