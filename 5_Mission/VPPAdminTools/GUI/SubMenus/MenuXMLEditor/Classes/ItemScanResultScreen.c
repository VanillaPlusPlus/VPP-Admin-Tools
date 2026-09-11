class ItemScanResultScreen: ScriptedWidgetEventHandler
{
    protected Widget    	m_EntryWidget;
    protected TextWidget    m_SomeText;
    protected ButtonWidget  m_BtnClose;
    protected ButtonWidget  m_btnDeleteObj;
    protected ButtonWidget  m_btnDeleteAll;
    protected MapWidget     m_map;
    protected TextListboxWidget m_itemsList;
    protected EditBoxWidget m_SearchInputBox;
	
    void ItemScanResultScreen()
    {
		m_EntryWidget = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.itemscanresultscreen, null);
		m_SomeText      = TextWidget.Cast(m_EntryWidget.FindAnyWidget("SomeText"));
		m_BtnClose      = ButtonWidget.Cast(m_EntryWidget.FindAnyWidget("BtnClose"));
		m_map      		= MapWidget.Cast(m_EntryWidget.FindAnyWidget("map"));
		m_itemsList		= TextListboxWidget.Cast(m_EntryWidget.FindAnyWidget("itemsList"));
		m_SearchInputBox = EditBoxWidget.Cast(m_EntryWidget.FindAnyWidget("SearchInputBox"));
		m_btnDeleteObj   = ButtonWidget.Cast(m_EntryWidget.FindAnyWidget("btnDeleteObj"));

		m_btnDeleteAll	 = ButtonWidget.Cast(m_EntryWidget.FindAnyWidget("btnDeleteAll"));
		GetVPPUIManager().HookConfirmationDialog(m_btnDeleteAll, m_EntryWidget, this, "ActionDeleteAll", DIAGTYPE.DIAG_YESNO, "#VSTR_LBL_DELETE_ALL", "#VSTR_XML_DELALL_BODY", true);

		m_EntryWidget.SetHandler(this);
		m_EntryWidget.SetSort(110,true);
		m_map.SetSort(115,true);

		WidgetEventHandler.GetInstance().RegisterOnDoubleClick(m_map, this, "MapDoubleClick");
    }
	
	void ~ItemScanResultScreen()
	{
		if (m_EntryWidget != null)
			m_EntryWidget.Unlink();
	}

	void ShowHide(bool state)
	{
		m_EntryWidget.Show(state); //false = hide
	}
	
	void StoreItemData(string name, vector pos, int lowBits, int highBits)
	{
		//Add to list of items
		m_itemsList.AddItem(name, new Param3<vector, int, int>(pos, lowBits, highBits), 0);
		DrawMarker(name, pos);
	}

	void DrawMarker(string name, vector pos)
	{
		m_map.AddUserMark(pos, name, ARGB(255,0,215,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypoint_CA.paa");
	}

	void DrawAllMarkers()
	{
		ClearMarkers();
		int total = m_itemsList.GetNumItems();
		for (int i = 0; i < total; ++i)
		{
			Param3<vector, int, int> rowData;
			m_itemsList.GetItemData(i, 0, rowData);

			if (rowData == NULL){
				m_itemsList.RemoveRow(i); //if for someone reason we have an invalid data entry
				continue;
			}

			string objType;
			m_itemsList.GetItemText(i, 0, objType);
			DrawMarker(objType, rowData.param1);
		}
		m_itemsList.SelectRow(-1);
	}

	void ActionDeleteAll(int result)
	{
		if (result != DIAGRESULT.YES)
			return;

		int total = m_itemsList.GetNumItems();
		for (int i = 0; i < total; ++i)
		{
			Param3<vector, int, int> rowData;
			m_itemsList.GetItemData(i, 0, rowData);
			if (rowData == NULL)
				continue;

			GetRPCManager().VSendRPC("RPC_XMLEditor", "DeleteCEObject", new Param2<int, int>(rowData.param2, rowData.param3), true, NULL);
		}
		m_itemsList.ClearItems();
		ClearMarkers();
	}
	
	void ClearMarkers()
	{
		m_map.ClearUserMarks();
	}
	
	void SetResultText(string txt)
	{
		m_SomeText.SetText(txt);
	}

	void MapDoubleClick(Widget w, int x, int y, int button)
	{
		if (button == MouseState.LEFT)
		{
			GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteTeleportPlayers", new Param3<ref array<string>,string,vector>({"self"}, "", ScreenToWorld()), true);
		}
	}

	vector ScreenToWorld()
	{
		vector world_pos,ScreenToMap,dir,from,to;
		dir = GetGame().GetPointerDirection();
	    from = GetGame().GetCurrentCameraPosition();
	    to = from + ( dir * 1000 );
		world_pos = GetGame().GetScreenPos( to );
	    ScreenToMap = m_map.ScreenToMap( world_pos );
	    return ScreenToMap;
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == m_SearchInputBox)
		{
			string strSearch = m_SearchInputBox.GetText();
        	strSearch.ToLower();

        	for (int i = 0; i < m_itemsList.GetNumItems(); ++i)
        	{
        		string entryName;
        		m_itemsList.GetItemText(i, 0, entryName);
        		entryName.ToLower();

        		m_itemsList.SetItemColor(i, 0, ARGB(255,255,255,255)); //default

        		if (strSearch != string.Empty && entryName.Contains(strSearch))
        			m_itemsList.SetItemColor(i, 0, ARGB(255,0,255,0));
        	}
		}
		return super.OnChange(w, x, y, finished);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_BtnClose)
		{
			delete this;
			return true;
		}

		int row;
		Param3<vector, int, int> rowData;

		if (w == m_itemsList)
		{
			row = m_itemsList.GetSelectedRow();
			if (row > -1)
			{
				m_itemsList.GetItemData(row, 0, rowData);

				float scale = Math.Clamp(m_map.GetScale() / 3, 0.1, 2.5);
				m_map.SetScale(scale);
				m_map.SetMapPos(rowData.param1);
			}
		}

		if (w == m_btnDeleteObj)
		{
			row = m_itemsList.GetSelectedRow();
			if (row <= -1)
			{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_XML_NOITEM");
				return false;
			}

			m_itemsList.GetItemData(row, 0, rowData);
			if (rowData != NULL)
			{
				m_itemsList.RemoveRow(row);
				GetRPCManager().VSendRPC("RPC_XMLEditor", "DeleteCEObject", new Param2<int, int>(rowData.param2, rowData.param3), true, NULL);
				DrawAllMarkers();
				return true;
			}
			else
			{
				GetVPPUIManager().DisplayError("#VSTR_XML_ERR_NOCACHE");
				return false;
			}
			return true;
		}

		return false;
	}
};