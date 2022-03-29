class MenuXMLEditor extends AdminHudSubMenu
{
	private bool 			  m_Loaded;
	private TextListboxWidget m_ItemListBoxXML;
	private ButtonWidget      m_BtnXMLEditorApply;
	private ButtonWidget      m_BtnLoadSelected;
	private ButtonWidget      m_BtnGetStats;
	private ItemPreviewWidget m_ItemPreviewXML;
	private EditBoxWidget     m_InputNominal;
	private EditBoxWidget     m_InputLifetime;
	private EditBoxWidget     m_InputRestock;
	private EditBoxWidget     m_InputMin;
	private EditBoxWidget     m_InputQuantmin;
	private EditBoxWidget     m_InputQuantmax;
	private EditBoxWidget     m_InputCost;
	private EditBoxWidget     m_SearchBoxXML;
	private CheckBoxWidget    m_chkEnablePreview;
	private ImageWidget 	  m_ImgInfoXMLToolTip;
	private CheckBoxWidget 	  m_filterFromXml;

	private Widget 				  m_FilesDropDownWidget;
	protected ref VPPDropDownMenu m_FilesDropDown;
	private ref array<string> 	  m_FilePaths;

	private ref array<string> 	  m_ClassTypes; //items store from server, for search filter
	
	ref ItemScanResultScreen 	m_MapScreen;
	
	private EntityAI 	  	  m_PreviewObject;
	private int 	 		  m_RotationX;
	private int 	 		  m_RotationY;
	private vector   		  m_ItemOrientation;
	private int    			  m_searchBoxCount;
	private int    			  prevRow;
	
	private ref map<string, string> m_TypesFilesData; //path, name we get from server
	private ref array<ref Param3<string,string,int>> m_cacheData;
	private string 									 m_selectedType;
	
	void MenuXMLEditor()
	{
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "HandleDetails", this);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "HandleStats", this);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "HandleTypesFiles", this);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "HandleTypesList", this);
	}
	
	void ~MenuXMLEditor()
	{
		if (m_PreviewObject != null)
		{
			GetGame().ObjectDelete(m_PreviewObject);
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.UpdatePreviewWidget);
		}
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuXMLEditor);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_InputNominal = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputNominal") );
		m_InputLifetime = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputLifetime") );
		m_InputRestock = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputRestock") );
		m_InputMin = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputMin") );
		m_InputQuantmin = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputQuantmin") );
		m_InputQuantmax = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputQuantmax") );
		m_InputCost = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputCost") );
		m_SearchBoxXML = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SearchBoxXML") );
		m_chkEnablePreview = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "chkEnablePreview") );
		
		m_ImgInfoXMLToolTip = ImageWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ImgInfoXMLToolTip") );

		m_BtnXMLEditorApply = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnXMLEditorApply") );
		GetVPPUIManager().HookConfirmationDialog(m_BtnXMLEditorApply, M_SUB_WIDGET,this,"ApplyChanges", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_NOTICE", "#VSTR_TOOLTIP_XML_EDITORCHN");
		
		m_BtnLoadSelected   = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnLoadSelected") );
		m_BtnGetStats       = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnGetStats") );
		
		m_ItemPreviewXML = ItemPreviewWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemPreviewXML") );
		m_ItemListBoxXML = TextListboxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemListBoxXML") );
		
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdatePreviewWidget, 100, true);
		
		//--
		m_filterFromXml = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("filterFromXml"));

		m_FilesDropDownWidget  = Widget.Cast(M_SUB_WIDGET.FindAnyWidget("ScriptsDropDown"));
		m_FilesDropDown  = new VPPDropDownMenu(m_FilesDropDownWidget, "Found files...");
		m_FilesDropDown.m_OnSelectItem.Insert(OnSelectFile);
		m_FilesDropDown.SetIndex(-1); //nothing selected
		//--

		ToolTipHandler toolTipMenu;
		m_ImgInfoXMLToolTip.GetScript(toolTipMenu);
		toolTipMenu.SetTitle("#VSTR_TOOLTIP_TITLE");
		toolTipMenu.SetContentText("Edit your types.xml file directly from here. Changes (to the CLE) after saving won't occur until next server restart."); //#VSTR_XML_MENU_TOOLTIP

		ToolTipHandler toolTip;
		
		m_InputNominal.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_XML_TITLE_NOMINAL");
		toolTip.SetContentText("#VSTR_XML_NOMINAL");
		//--
		m_InputLifetime.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_XML_TITLE_LIFETIME");
		toolTip.SetContentText("#VSTR_XML_LIFETIME");
		//--
		m_InputRestock.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_XML_TITLE_RESTOCK");
		toolTip.SetContentText("#VSTR_XML_RESTOCK");
		//--
		m_InputMin.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_XML_TITLE_MIN");
		toolTip.SetContentText("#VSTR_XML_MIN");
		//--
		m_InputQuantmin.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_XML_TITLE_QUANTMIN");
		toolTip.SetContentText("#VSTR_XML_QUANTMIN");
		//--
		m_InputQuantmax.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_XML_TITLE_QUANTMAX");
		toolTip.SetContentText("#VSTR_XML_QUNATMAX");
		//--
		m_InputCost.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_XML_TITLE_COST");
		toolTip.SetContentText("#VSTR_XML_COST");
		//--
		m_Loaded = true;
		UpdateFilter();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "GetTypesFiles", NULL, true, NULL);
	}
	
	override void OnAdminHudOpened()
	{
		super.OnAdminHudOpened();
		if (!IsSubMenuVisible() || !m_Loaded)
			return;

		if (m_MapScreen)
			m_MapScreen.ShowHide(true);
	}

	override void ShowSubMenu()
	{
		super.ShowSubMenu();
		//Do Something
	}
	
	void OnSelectFile(int index)
	{
		if (m_FilePaths == NULL)
			return;
		
		m_FilesDropDown.SetText(m_FilePaths[index]);
		m_FilesDropDown.SetIndex(index);
		m_FilesDropDown.Close();
		m_filterFromXml.SetChecked(false);
		m_ClassTypes = {};
		UpdateFilter();
		ResetInputBoxes();
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() || !m_Loaded) return;
		
		//--
		m_BtnXMLEditorApply.Enable(m_ItemListBoxXML.GetSelectedRow() > -1 || m_cacheData != null);
		m_BtnLoadSelected.Enable(m_ItemListBoxXML.GetSelectedRow() > -1 || m_cacheData != null);
		m_BtnGetStats.Enable(m_ItemListBoxXML.GetSelectedRow() > -1 || m_cacheData != null);
		//---
				
		int newSrchCount = m_SearchBoxXML.GetText().Length();
		if (newSrchCount != m_searchBoxCount)
		{
			UpdateFilter();
			m_searchBoxCount = newSrchCount;
		}
	}
	
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		string typeName;
		if (w == m_BtnLoadSelected)
		{
			if (m_ItemListBoxXML.GetSelectedRow() == -1)
			{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_XML_NOITEM");
				return false;
			}
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_WAIT_INFO");
			m_ItemListBoxXML.GetItemText(m_ItemListBoxXML.GetSelectedRow(), 0, typeName);
			GetRPCManager().VSendRPC("RPC_XMLEditor", "GetDetails", new Param2<string, string>(typeName, GetLoadedFilePath()), true, null);
			return true;
		}

		if (w == m_BtnGetStats)
		{
			m_ItemListBoxXML.GetItemText(m_ItemListBoxXML.GetSelectedRow(),0,typeName);
			if (typeName == "")
			{
				GetVPPUIManager().DisplayError("#VSTR_XML_ERR_SELECTFIRST");
				return false;
			}
			m_MapScreen = new ItemScanResultScreen();
			GetRPCManager().VSendRPC("RPC_XMLEditor", "GetScanInfo", new Param1<string>(typeName), true, null);
			return true;
		}

		if (w == m_filterFromXml)
		{
			if (m_filterFromXml.IsChecked())
			{
				GetRPCManager().VSendRPC("RPC_XMLEditor", "GetTypesFromFile", new Param1<string>(GetLoadedFilePath()), true, null);
			}
			return true;
		}
		return super.OnClick(w, x, y, button);
	}

	string GetLoadedFilePath()
	{
		int selectedIdx = m_FilesDropDown.GetIndex();
		if (selectedIdx > -1)
		{
			return m_TypesFilesData.GetKey(selectedIdx);
		}
		return "$mission:db/types.xml"; //the default will always be this
	}
	
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		super.OnMouseButtonDown(w, x, y, button);
		if (w == m_ItemPreviewXML)
		{
			GetGame().GetDragQueue().Call(this, "UpdateItemRotation");
			g_Game.GetMousePos(m_RotationX, m_RotationY);
			return true;
		}
		return false;
	}
	
	void ResetInputBoxes()
	{
		m_InputNominal.SetText("0");
		m_InputNominal.Enable(true);
		m_InputLifetime.SetText("0");
		m_InputLifetime.Enable(true);
		m_InputRestock.SetText("0");
		m_InputRestock.Enable(true);
		m_InputMin.SetText("0");
		m_InputMin.Enable(true);
		m_InputQuantmin.SetText("0");
		m_InputQuantmin.Enable(true);
		m_InputQuantmax.SetText("0");
		m_InputQuantmax.Enable(true);
		m_InputCost.SetText("0");
		m_InputCost.Enable(true);
	}
	
	void HandleStats(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref map<string,vector>> data;
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Client)
		{
			map<string,vector> result = data.param1;
			if (m_MapScreen != null)
				delete m_MapScreen;
			
			m_MapScreen = new ItemScanResultScreen();
			foreach(string name, vector pos: result)
			{
				m_MapScreen.DrawMarker(name,pos);
			}
			m_MapScreen.SetResultText("#VSTR_NOTIFY_TOTALSCANNED"+ " ["+ result.Count() +"]");
		}
	}
	
	void HandleDetails(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string, ref array<ref Param3<string,string,int>>> data; //elemnt type, value, index in file
		if (!ctx.Read(data)) return;
		
	 	if ( type == CallType.Client )
		{
			ResetInputBoxes();
			m_cacheData    = data.param2;
			m_selectedType = data.param1;

			foreach(Param3<string,string,int> param : m_cacheData)
			{
				switch(param.param1)
				{
					case "nominal":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputNominal.SetText("#VSTR_XML_ERR_NOT_DEF");
						m_InputNominal.Enable(false);
					}else{
						m_InputNominal.SetText(param.param2);
					}
					break;
					
					case "lifetime":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputLifetime.SetText("#VSTR_XML_ERR_NOT_DEF");
						m_InputLifetime.Enable(false);
					}else{
						m_InputLifetime.SetText(param.param2);
					}
					break;
					
					case "restock":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputRestock.SetText("#VSTR_XML_ERR_NOT_DEF");
						m_InputRestock.Enable(false);
					}else{
						m_InputRestock.SetText(param.param2);
					}
					break;
					
					case "min":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputMin.SetText("#VSTR_XML_ERR_NOT_DEF");
						m_InputMin.Enable(false);
					}else{
						m_InputMin.SetText(param.param2);
					}
					break;
					
					case "quantmin":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputQuantmin.SetText("#VSTR_XML_ERR_NOT_DEF");
						m_InputQuantmin.Enable(false);
					}else{
						m_InputQuantmin.SetText(param.param2);
					}
					break;
					
					case "quantmax":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputQuantmax.SetText("#VSTR_XML_ERR_NOT_DEF");
						m_InputQuantmax.Enable(false);
					}else{
						m_InputQuantmax.SetText(param.param2);
					}
					break;
					
					case "cost":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputCost.SetText("#VSTR_XML_ERR_NOT_DEF");
						m_InputCost.Enable(false);
					}else{
						m_InputCost.SetText(param.param2);
					}
					break;
				}
			}
			GetVPPUIManager().DisplayNotification("#VSTR_XML_SUCCESS_RETRIVE");
		}
	}
	
	void HandleTypesFiles(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Client)
		{
			Param1<ref map<string, string>> data; //filePath, name
			if (!ctx.Read(data))
				return;

			m_TypesFilesData = data.param1;
			m_FilesDropDown.RemoveAllElements();
			m_FilePaths = {};
			foreach(string filePath, string name : m_TypesFilesData)
			{
				m_FilePaths.Insert(filePath);
				m_FilesDropDown.AddElement(name);
			}
		}
	}

	void HandleTypesList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Client)
		{
			Param1<ref array<string>> data;
			if (!ctx.Read(data))
				return;

			m_ClassTypes = {};
			m_ClassTypes.Copy(data.param1);
			UpdateFilter();
		}
	}

	void UpdateCacheData()
	{
		foreach(ref Param3<string,string,int> param : m_cacheData)
		{
			switch(param.param1)
			{
				case "nominal":
				if (param.param2 != "NOT DEFINED" && param.param3 != -1)
					param.param2 = m_InputNominal.GetText();
				break;
				
				case "lifetime":
				if (param.param2 != "NOT DEFINED" && param.param3 != -1)
					param.param2 = m_InputLifetime.GetText();
				break;
				
				case "restock":
				if (param.param2 != "NOT DEFINED" && param.param3 != -1)
					param.param2 = m_InputRestock.GetText();
				break;
				
				case "min":
				if (param.param2 != "NOT DEFINED" && param.param3 != -1)
					param.param2 = m_InputMin.GetText();
				break;
				
				case "quantmin":
				if (param.param2 != "NOT DEFINED" && param.param3 != -1)
					param.param2 = m_InputQuantmin.GetText();
				break;
				
				case "quantmax":
				if (param.param2 != "NOT DEFINED" && param.param3 != -1)
					param.param2 = m_InputQuantmax.GetText();
				break;
				
				case "cost":
				if (param.param2 != "NOT DEFINED" && param.param3 != -1)
					param.param2 = m_InputCost.GetText();
				break;
			}
		}
	}
	
	void ApplyChanges(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			if (m_cacheData == null || m_selectedType == "")
			{
				GetVPPUIManager().DisplayError("#VSTR_XML_NOTIFY_ERR");
				return;
			}
			UpdateCacheData();
			GetRPCManager().VSendRPC("RPC_XMLEditor", "EditElemets", new Param3<string, string, ref array<ref Param3<string,string,int>>>(GetLoadedFilePath(), m_selectedType, m_cacheData), true, null);
			
			m_selectedType = "";
			m_cacheData    = null;
			ResetInputBoxes();
		}
	}
	
	void UpdateFilter()
	{
		m_ItemListBoxXML.ClearItems();

        TStringArray cfgPaths = new TStringArray;
        cfgPaths.Insert( "CfgVehicles" );
        cfgPaths.Insert( "CfgWeapons" );
        cfgPaths.Insert( "CfgMagazines" );

        string strSearch = m_SearchBoxXML.GetText();
        strSearch.ToLower();
        string strName;
        string lowerCasedName;

        if (m_filterFromXml.IsChecked())
        {
        	for (int i = 0; i < m_ClassTypes.Count(); ++i)
        	{
        		strName = m_ClassTypes[i];
        		lowerCasedName = strName;
	            lowerCasedName.ToLower();
	            if ((strSearch != "" && (!lowerCasedName.Contains(strSearch)))) 
	                continue;

	            m_ItemListBoxXML.AddItem(strName, NULL, 0);
        	}
        }
        else
        {
        	for(int x = 0; x < cfgPaths.Count(); ++x)
	        {
	            string Config_Path = cfgPaths.Get(x);
	            int nClasses = g_Game.ConfigGetChildrenCount(Config_Path);

	            for (int nClass = 0; nClass < nClasses; ++nClass)
	            {
	                GetGame().ConfigGetChildName(Config_Path, nClass, strName);

	                int scope = GetGame().ConfigGetInt(Config_Path + " " + strName + " scope");

	                if (scope == 0 || scope == 1)
	                    continue;

	                lowerCasedName = strName;
	                lowerCasedName.ToLower();
	                if ((strSearch != "" && (!lowerCasedName.Contains(strSearch)))) 
	                {
	                    continue;
	                }
	                m_ItemListBoxXML.AddItem(strName, NULL, 0);
	            }
	        }
        }
	}
	
	void UpdatePreviewWidget()
	{
		if (!IsSubMenuVisible() || !m_Loaded) return;
		
		if (m_chkEnablePreview.IsChecked())
		{
			int oRow = m_ItemListBoxXML.GetSelectedRow();
			string ItemClassName;
			
			if (oRow != -1 && oRow != prevRow)
			{
				m_ItemListBoxXML.GetItemText(oRow, 0, ItemClassName);
				if (GetGame().IsKindOf( ItemClassName, "dz_lightai" ) || ItemClassName == "") return;
				
				if (m_PreviewObject != null)
					GetGame().ObjectDelete(m_PreviewObject);
				
				m_PreviewObject = EntityAI.Cast(GetGame().CreateObject(ItemClassName,vector.Zero,true,false,false));
				if (m_PreviewObject != null)
				{
					m_ItemPreviewXML.SetItem( m_PreviewObject );
					m_ItemPreviewXML.SetModelPosition( Vector(0,0,0.5) );
					m_ItemPreviewXML.SetModelOrientation( Vector(0,0,0) );
					m_ItemPreviewXML.SetView( m_ItemPreviewXML.GetItem().GetViewIndex() );
					m_ItemPreviewXML.Show(true);
				}else{
					m_ItemPreviewXML.Show(false);
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
			orientation        = m_ItemPreviewXML.GetModelOrientation();
			m_ItemOrientation  = m_ItemPreviewXML.GetModelOrientation();
		}
		else orientation = m_ItemOrientation;
		
		orientation[0] = orientation[0] + (m_RotationY - mouse_y);
		orientation[1] = orientation[1] - (m_RotationX - mouse_x);

		m_ItemPreviewXML.SetModelOrientation( orientation );

		if ( !is_dragging )
			m_ItemOrientation = orientation;
	}
};