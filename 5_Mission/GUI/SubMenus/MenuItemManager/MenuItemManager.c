class MenuItemManager extends AdminHudSubMenu
{
	private ref CustomGridSpacer 	 m_LastGrid;
	private GridSpacerWidget         m_ParentGrid;
	private ImageWidget              m_ImgInfoAddPreset;
	private EditBoxWidget     		 m_SearchInputBox;
	private XComboBoxWidget   		 m_ComboCatagory;
	private TextListboxWidget 		 m_ItemListBox;
	private ScrollWidget 		 	 m_ScrollerItemList;
	private ButtonWidget             m_BtnSaveChanges;
	private ButtonWidget             m_BtnSpawnPreset;
	private ButtonWidget             m_BtnCreatePreset;
	private ButtonWidget             m_BtnDeletePreset;
	private ButtonWidget             m_btnRefresh;
	private XComboBoxWidget          m_ComboCondition;
	private XComboBoxWidget          m_ComboPlacement;
	private EditBoxWidget          	 m_InputQuantity;
	private CheckBoxWidget           m_ChkOnSelectedPlayers;
	private CheckBoxWidget           m_chkBoxPreview;
	private Widget 					 m_SavedPresetsWidget;
	protected ref VPPDropDownMenu 	 m_SavedPresetsDropDown;
	private ref PopUpCreatePreset  	 m_PopUpPresetCreate;
	private ref Widget           	 m_PopUpPresetCreateWidget;
	private Widget 					 m_Main;
	private ItemPreviewWidget 		 m_ItemPreview;
	private EntityAI 	  	  		 m_PreviewObject;
	private int 	 		  		 m_RotationX;
	private int 	 		  		 m_RotationY;
	private vector   		  		 m_ItemOrientation;
	
	private ref array<ref PresetItemEntry>  m_Entries;
	private ref array<ref CustomGridSpacer> m_DataGrids;
	private ref array<ref PresetItemData>   m_PresetItemData; //holds all presets items from server
	private ref PresetItemData              m_CurrentPresetData;
	private const ref array<string> m_CatagoryItems = {"All","edible_base","transport","weapon_base","clothing_base","house","dz_lightai"};
	private string m_CurrentCatagory;
	private int    m_searchBoxCount = 0;
	private bool   m_loaded;
	private int    prevRow;
	
	void MenuItemManager()
	{
		m_DataGrids = new array<ref CustomGridSpacer>;
		m_Entries   = new array<ref PresetItemEntry>;
		m_PresetItemData = new array<ref PresetItemData>;
		
		/* RPCs */
		GetRPCManager().AddRPC("RPC_MenuItemManager", "HandleData", this);
		//-------
	}
	
	void ~MenuItemManager()
	{
		if (m_PreviewObject != null)
		GetGame().ObjectDelete(m_PreviewObject);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.UpdatePreviewWidget);
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		
		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/ItemManagerUI/MenuItemManager.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_Main		  	 = M_SUB_WIDGET.FindAnyWidget( "Main");
		m_ParentGrid     = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ParentGrid"));
		m_SearchInputBox = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SearchInputBox") );
		m_ComboCatagory  = XComboBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ComboCatagory") );
		m_ItemListBox    = TextListboxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemListBox") );
		m_ScrollerItemList  = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ScrollerItemList") );
		m_ItemPreview    = ItemPreviewWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemPreview") );
		m_BtnSaveChanges = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnSaveChanges") );
		m_BtnSpawnPreset = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnSpawnPreset") );
		m_BtnCreatePreset = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnCreatePreset") );
		m_BtnDeletePreset = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnDeletePreset") );
		m_btnRefresh 	  = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "btnRefresh") );
		m_ComboCondition = XComboBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ComboCondition") );
	 	m_ComboPlacement = XComboBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ComboPlacement") );
	 	m_InputQuantity  = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputQuantity") );
		m_ChkOnSelectedPlayers = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ChkOnSelectedPlayers") );
		m_chkBoxPreview  = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "chkBoxPreview") );

		m_ImgInfoAddPreset = ImageWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ImgInfoAddPreset") );
		autoptr ToolTipHandler toolTip;
		m_ImgInfoAddPreset.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_TOOLTIP_TITLE");
		toolTip.SetContentText("#VSTR_TOOLTIP_ITEMMGR");
		
		m_SavedPresetsWidget = M_SUB_WIDGET.FindAnyWidget( "PresetsDropDown");
		m_SavedPresetsDropDown = new VPPDropDownMenu( m_SavedPresetsWidget, "#VSTR_TITLE_SAVEDPRESETS" );
		m_SavedPresetsDropDown.m_OnSelectItem.Insert( OnSelectItemPreset );
		
		foreach(string catagoryType : m_CatagoryItems){
			m_ComboCatagory.AddItem(catagoryType);
		}
		SetSearchCatagory();
		UpdateFilter();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdatePreviewWidget, 100, true);
		
		GetRPCManager().SendRPC("RPC_VPPItemManager", "GetData", null, true, null);
		m_loaded = true;
	}
	
	override void HideBrokenWidgets(bool state)
	{
		m_ScrollerItemList.Show(!state);
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() && !m_loaded) return; //Update cancels if sub menu is not visible.
		
		int newSrchCount = m_SearchInputBox.GetText().Length();
		if (newSrchCount != m_searchBoxCount)
		{
			//Update Filter
			UpdateFilter();
			m_searchBoxCount = newSrchCount;
		}
		
		//Save Changes button & delete button
		if (m_SavedPresetsDropDown.GetIndex() != -1){
			m_BtnSaveChanges.Enable(true);
			m_BtnDeletePreset.Enable(true);
		}else{
			m_BtnSaveChanges.Enable(false);
			m_BtnDeletePreset.Enable(false);
		}
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case m_SearchInputBox:
			UpdateFilter();
			break;
			
			case m_ComboCatagory:
			SetSearchCatagory();
			UpdateFilter();
			break;
			
			case m_BtnSaveChanges:
			if (m_CurrentPresetData != null)
				GetRPCManager().SendRPC("RPC_VPPItemManager", "EditPreset", new Param1<ref PresetItemData>(m_CurrentPresetData), true, null);
			break;
			
			case m_BtnSpawnPreset:
			if (m_SavedPresetsDropDown.GetText() != "#VSTR_TITLE_SAVEDPRESETS")
				RequestSpawn();
			else
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_NOPRESET");
			break;
			
			case m_BtnDeletePreset:
			if (m_SavedPresetsDropDown.GetIndex() != -1 && m_SavedPresetsDropDown.GetText() != "#VSTR_TITLE_SAVEDPRESETS" || m_SavedPresetsDropDown.GetText() != "")
			{
				DeletePreset(m_SavedPresetsDropDown.GetText());
			}else{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_NOPRESET");
			}
			break;
			
			case m_BtnCreatePreset:
			if (m_PopUpPresetCreateWidget != null && m_PopUpPresetCreate != null)
			{
				m_PopUpPresetCreateWidget.Unlink();
				delete m_PopUpPresetCreate;
			}
			m_PopUpPresetCreateWidget = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/ItemManagerUI/PopUpCreatePreset.layout", m_Main);
			m_PopUpPresetCreateWidget.GetScript(m_PopUpPresetCreate);
			m_PopUpPresetCreate.Init(this);
			break;
			
			case m_btnRefresh:
			m_CurrentPresetData = null;
			GetRPCManager().SendRPC("RPC_VPPItemManager", "GetData", null, true, null);
			break;
			
			case m_chkBoxPreview:
				if (m_chkBoxPreview.IsChecked())
					UpdatePreviewWidget();
			break;
		}
		return false;
	}
	
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		super.OnMouseButtonDown(w, x, y, button);
		if (w == m_ItemPreview)
		{
			GetGame().GetDragQueue().Call(this, "UpdateItemRotation");
			g_Game.GetMousePos(m_RotationX, m_RotationY);
			return true;
		}
		return false;
	}
	
	void SaveNewPreset(string presetName)
	{
		GetRPCManager().SendRPC("RPC_VPPItemManager", "AddPreset", new Param1<string>(presetName), true, null);
	}
	
	bool CheckDuplicatePreset(string presetName)
	{
		foreach(PresetItemData preset : m_PresetItemData)
		{
			if (preset.GetPresetName() == presetName)
				return true;
		}
		return false;
	}
	
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (w == m_ItemListBox)
		{
			if( button == MouseState.LEFT)
			{
				string typeName;
				m_ItemListBox.GetItemText(m_ItemListBox.GetSelectedRow(),0,typeName);
	
				if (g_Game.IsLeftCtrlDown())
				{
					//Add item to selected preset
					if (m_SavedPresetsDropDown.GetIndex() == -1 || m_SavedPresetsDropDown.GetText() == "#VSTR_TITLE_SAVEDPRESETS" && m_CurrentPresetData == null){
						GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_NOTSELECTPRESET");
						return false;
					}
					m_CurrentPresetData.AddItem(typeName,false);
					ReloadPresetData();
				}else{
					RequestSpawn(true,typeName);
				}
			}
			return true;
		}
		return false;
	}
	
	void DeletePreset(string presetName)
	{
		GetRPCManager().SendRPC("RPC_VPPItemManager", "DeletePreset", new Param1<string>(presetName), true, null);
	}
	
	void RequestSpawn(bool singleItem = false, string typeName = "")
	{
		int quantity = m_InputQuantity.GetText().ToInt();
		if (quantity <= 0) quantity = -1; //-1 == MAX
		
		int condition = m_ComboCondition.GetCurrentItem();
		int placementType = m_ComboPlacement.GetCurrentItem();
		string presetName = m_SavedPresetsDropDown.GetText();
		
		vector pos;
		switch(placementType)
		{
			case PlacementTypes.ON_GROUND:
			pos = GetGame().GetPlayer().GetPosition();
			break;
			
			case PlacementTypes.IN_INVENTORY:
			pos = "0 0 0";
			break;
			
			case PlacementTypes.AT_CROSSHAIR:
			pos = g_Game.GetCursorPos();
			break;
		}
		array<string> trgIDs = new array<string>;
		if (m_ChkOnSelectedPlayers.IsChecked())
		{
			autoptr MenuPlayerManager pManager = MenuPlayerManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPlayerManager));
			if (pManager == null)
			{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_SPAWN_PRESET");
				return;
			}
			
			trgIDs = pManager.GetSelectedPlayersIDs();
			if (trgIDs.Count() <= 0)
			{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_SPAWN_PRESET_NOPLAYER");
				return;
			}
		}
		

		if (singleItem && typeName != "")
			GetRPCManager().SendRPC("RPC_VPPItemManager", "SpawnItem", new Param1<ref ItemSpawnParams>(new ItemSpawnParams(typeName, pos, quantity, condition, placementType, trgIDs)), true, null);
			else
			GetRPCManager().SendRPC("RPC_VPPItemManager", "RemoteSpawnPreset", new Param1<ref ItemSpawnParams>(new ItemSpawnParams(presetName, pos, quantity, condition, placementType, trgIDs)), true, null);
	}
	
	void ReloadPresetData()
	{
		ClearEntriesList();
		InitPresetItems(m_CurrentPresetData);
	}
	
	/*
	\ RPC Section
	*/
	void HandleData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<ref PresetItemData>> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Client)
		{
			m_PresetItemData = data.param1;
			ClearEntriesList();
			m_SavedPresetsDropDown.RemoveAllElements();
			foreach(PresetItemData item : m_PresetItemData){
				m_SavedPresetsDropDown.AddElement(item.GetPresetName());
			}
		}
	}
	/*
	\------------
	*/
	
	/*
		use this function to add items onto the list when a preset is selected!
	*/
	void InitPresetItems(PresetItemData data)
	{
		foreach(CustomGridSpacer cusGrid : m_DataGrids){
			if (cusGrid != null)
				delete cusGrid;
		}
		m_DataGrids = new array<ref CustomGridSpacer>;
		
		//init first "page"
		m_DataGrids.Insert(new CustomGridSpacer(m_ParentGrid));
		m_LastGrid = m_DataGrids[0];
		AddEntry(data);
	}
	
	void AddEntry(ref PresetItemData item)
	{
		if(m_LastGrid.GetContentCount() == 100)
		{
			m_LastGrid = new CustomGridSpacer(m_ParentGrid);
		 	m_DataGrids.Insert(m_LastGrid);
		}
		
		if(m_LastGrid.GetContentCount() < 100)
		{
			ref PresetItemEntry entry;
			//Add Children
			autoptr array<string> childItems = item.GetItemTypes();
			foreach(string type : childItems)
			{
				if (type == "") continue;
				
				if (item.IsParent(type))
					entry = new PresetItemEntry(m_LastGrid.GetGrid(), item, item.GetParentType(),true);
				else
					entry = new PresetItemEntry(m_LastGrid.GetGrid(),item, type,false);

				m_LastGrid.AddWidget(entry.m_EntryBox);
				m_Entries.Insert(entry);
			}
		}
		
		m_ParentGrid.Update();
		m_LastGrid.GetGrid().Update();
	}
	
	void ClearEntriesList()
	{
		foreach(PresetItemEntry entry : m_Entries)
		{
			if (entry != null)
			m_Entries.RemoveItem(entry);
				delete entry;
		}
		m_Entries = new array<ref PresetItemEntry>;
	}
	
	void OnSelectItemPreset(int index)
	{
		autoptr PresetItemData selected = m_PresetItemData.Get(index);
		m_SavedPresetsDropDown.SetText(selected.GetPresetName());
		m_SavedPresetsDropDown.Close();
		m_CurrentPresetData = selected;
		ClearEntriesList();
		InitPresetItems(selected);
	}
	
	void UpdatePreviewWidget()
	{
		if (!IsSubMenuVisible() && !m_loaded) return;
		
		if (m_chkBoxPreview.IsChecked())
		{
			int oRow = m_ItemListBox.GetSelectedRow();
			string ItemClassName;
			
			if (oRow != -1 && oRow != prevRow)
			{
				m_ItemListBox.GetItemText(oRow, 0, ItemClassName);
				if (GetGame().IsKindOf( ItemClassName, "dz_lightai" ) || ItemClassName == "") return;
				
				if (m_PreviewObject != null)
				{
					m_ItemPreview.SetItem(null);
					GetGame().ObjectDelete(m_PreviewObject);
				}
				
				m_PreviewObject = EntityAI.Cast(GetGame().CreateObject(ItemClassName,vector.Zero,true,false,false));
				if (m_PreviewObject != null)
				{
					m_ItemPreview.SetItem( m_PreviewObject );
					m_ItemPreview.SetModelPosition( Vector(0,0,0.5) );
					m_ItemPreview.SetModelOrientation( Vector(0,0,0) );
					m_ItemPreview.SetView( m_ItemPreview.GetItem().GetViewIndex() );
					m_ItemPreview.Show(true);
				}else{
					m_ItemPreview.Show(false);
				}
				m_ItemOrientation = Vector(0,0,0);
				prevRow = oRow;
			}
		}
	}
	
	void UpdateItemRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		vector orientation;
		if (m_ItemOrientation[0] == 0 && m_ItemOrientation[1] == 0 && m_ItemOrientation[2] == 0)
		{
			orientation        = m_ItemPreview.GetModelOrientation();
			m_ItemOrientation  = m_ItemPreview.GetModelOrientation();
		}
		else orientation = m_ItemOrientation;
		
		orientation[0] = orientation[0] + (m_RotationY - mouse_y);
		orientation[1] = orientation[1] - (m_RotationX - mouse_x);

		m_ItemPreview.SetModelOrientation( orientation );

		if ( !is_dragging )
			m_ItemOrientation = orientation;
	}
	
	void UpdateFilter()
	{
		m_ItemListBox.ClearItems();

        TStringArray cfgPaths = new TStringArray;
        cfgPaths.Insert( "CfgVehicles" );
        cfgPaths.Insert( "CfgWeapons" );
        cfgPaths.Insert( "CfgMagazines" );
        /*cfgPaths.Insert( "CfgNonAIVehicles" );*/

        string strSearch = m_SearchInputBox.GetText();
        strSearch.ToLower();

        for (int x = 0; x < cfgPaths.Count(); ++x)
        {
            string Config_Path = cfgPaths.Get(x);
            int nClasses = g_Game.ConfigGetChildrenCount(Config_Path);

            for ( int nClass = 0; nClass < nClasses; ++nClass )
            {
                string strName;
                GetGame().ConfigGetChildName( Config_Path, nClass, strName );

                int scope = GetGame().ConfigGetInt( Config_Path + " " + strName + " scope" );

                if ( scope == 0 || scope == 1)
                    continue;

                string lowerCasedName = strName;
                lowerCasedName.ToLower();
                if ( m_CurrentCatagory == "All" || GetGame().IsKindOf( lowerCasedName, m_CurrentCatagory ) )
                {
                    if ((strSearch != "" && (!lowerCasedName.Contains(strSearch)))) 
                    {
                        continue;
                    }
                    m_ItemListBox.AddItem( strName, NULL, 0 );
                }
            }
        }
	}
	
	void SetSearchCatagory()
	{
		if (m_ComboCatagory.GetCurrentItem() == -1){
			m_CurrentCatagory = "All";
		}else{
			m_CurrentCatagory = m_CatagoryItems.Get(m_ComboCatagory.GetCurrentItem());
		}
	}
	
	string GetSelectedItem()
	{
		int oRow = m_ItemListBox.GetSelectedRow();
		string ItemClassName;
		if (oRow != -1)
		{
			m_ItemListBox.GetItemText(oRow, 0, ItemClassName);
			return ItemClassName;
		}
		return "";
	}
};