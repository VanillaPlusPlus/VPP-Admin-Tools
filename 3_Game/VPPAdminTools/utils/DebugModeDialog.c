class DebugModeDialog: ScriptedWidgetEventHandler
{
	Widget m_Root;
	ButtonWidget m_SearchAgain;
	TextListboxWidget m_Missions;

	int m_RowSelected;

	void DebugModeDialog()
	{
	}

	void OnWidgetScriptInit(Widget w)
	{
		m_Root = w;
		m_Root.SetHandler(this);
		g_Game.GetUIManager().ShowCursor(true);
		m_SearchAgain = ButtonWidget.Cast(w.FindAnyWidget("SearchAgain"));
		m_Missions = TextListboxWidget.Cast(w.FindAnyWidget("Missions"));
	}

	void SetError(string title, string body)
	{
		TextWidget.Cast(m_Root.FindAnyWidget("TitleText")).SetText(title);
		RichTextWidget.Cast(m_Root.FindAnyWidget("ContentText")).SetText(body);
	}

	void RefreshSearch()
	{
		m_Missions.ClearItems();
		array<string> files = {};
		if (g_Game.SearchForCustomDebugMission(files))
		{
			foreach(string file: files)
				m_Missions.AddItem(file, NULL, 0);
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_SearchAgain)
		{
			RefreshSearch();
			return true;
		}
		return super.OnClick(w, x, y, button);
	}

	override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
	{
		m_RowSelected = row;
		return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (w == m_Missions)
		{
			if (m_RowSelected >= 0)
			{
				string path;
				m_Missions.GetItemText(m_RowSelected, 0, path);
				g_Game.PlayCustomMission(path);
			}

			return true;
		}
		return super.OnDoubleClick(w, x, y, button);
	}
};