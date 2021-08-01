class CustomGridSpacer
{
	private ref GridSpacerWidget  m_Grid;
	private ref GridSpacerWidget  m_Parent;
	private ref array<Widget> m_ContentWidget;
	
	void CustomGridSpacer(GridSpacerWidget parent)
	{
		m_ContentWidget = new array<Widget>;
		m_Grid  = GridSpacerWidget.Cast(GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/UIHelpers/CustomGridSpacer.layout", parent));
		m_Parent = parent;
		m_Parent.Update();
	}
	
	void ~CustomGridSpacer()
	{
		if (m_Grid != null)
			m_Grid.Unlink();
	}
	
	int GetContentCount()
	{
		return m_ContentWidget.Count();
	}
	
	GridSpacerWidget GetParnet()
	{
		return m_Parent;
	}
	
	void HideGrid(bool state)
	{
		m_Grid.Show(state);
		m_Grid.Update();
	}
	
	void AddWidget(Widget toAdd)
	{
		m_ContentWidget.Insert(toAdd);
		m_Grid.Update();
	}
	
	void ClearContent()
	{
		foreach(Widget widget : m_ContentWidget)
		{
			if (widget != null)
				widget.Unlink();
		}
		m_ContentWidget = new array<Widget>;
	}
	
	GridSpacerWidget GetGrid()
	{
		return m_Grid;
	}
};