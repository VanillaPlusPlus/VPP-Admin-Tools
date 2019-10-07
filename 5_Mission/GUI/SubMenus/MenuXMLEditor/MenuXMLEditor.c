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
	
	private ref ItemScanResultScreen m_MapScreen;
	
	private EntityAI 	  	  m_PreviewObject;
	private int 	 		  m_RotationX;
	private int 	 		  m_RotationY;
	private vector   		  m_ItemOrientation;
	private int    			  m_searchBoxCount;
	private int    			  prevRow;
	
	private ref array<ref Param3<string,string,int>> m_cacheData;
	private string 									 m_selectedType;
	
	void MenuXMLEditor()
	{
		GetRPCManager().AddRPC( "RPC_MenuXMLEditor", "HandleDetails", this );
		GetRPCManager().AddRPC( "RPC_MenuXMLEditor", "HandleStats", this );
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
		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/XMLEditorUI/MenuXMLEditor.layout");
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
		
		m_BtnXMLEditorApply = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnXMLEditorApply") );
		GetVPPUIManager().HookConfirmationDialog(m_BtnXMLEditorApply, M_SUB_WIDGET,this,"ApplyChanges", DIAGTYPE.DIAG_YESNO, "Important Notice!", "Are you sure you wish to save changes?\n\nPlease note that these changes don't take effect to the CE automatically, a copy of the newly edited types.xml will be saved in your servers' profile folder!");
		
		m_BtnLoadSelected   = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnLoadSelected") );
		
		m_BtnGetStats       = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnGetStats") );
		GetVPPUIManager().HookConfirmationDialog(m_BtnGetStats, M_SUB_WIDGET,this,"RequestStats", DIAGTYPE.DIAG_YESNO, "Important Notice!", "This feature uses heavy server resource, you may experience heavy lag!\n\nAre you sure you wish to continue?");
		
		
		m_ItemPreviewXML = ItemPreviewWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemPreviewXML") );
		m_ItemListBoxXML = TextListboxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemListBoxXML") );
		
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdatePreviewWidget, 100, true);
		
		autoptr ToolTipHandler toolTip;
		
		m_InputNominal.GetScript(toolTip);
		toolTip.SetTitle("Nominal:");
		toolTip.SetContentText("Items of this type are spawned to this value (assuming there is enough place in the world, CE queue is not pre-occupied and restock timer reaches 0)");
		//--
		m_InputLifetime.GetScript(toolTip);
		toolTip.SetTitle("Lifetime:");
		toolTip.SetContentText("Time in seconds before this type gets deleted (assuming no one touches it)");
		//--
		m_InputRestock.GetScript(toolTip);
		toolTip.SetTitle("Restock:");
		toolTip.SetContentText("If set to 0, CE tries to respawn type in bulks, if set to positive value, it is time in seconds before ONE item of this type is spawned again");
		//--
		m_InputMin.GetScript(toolTip);
		toolTip.SetTitle("Min:");
		toolTip.SetContentText("If amount of items of this type reach below this value in world, they get spawned to nominal again (they follow restock logic though)");
		//--
		m_InputQuantmin.GetScript(toolTip);
		toolTip.SetTitle("Quantmin:");
		toolTip.SetContentText("Min % for quantity (rags,mags,ammo,..)");
		//--
		m_InputQuantmax.GetScript(toolTip);
		toolTip.SetTitle("Quantmax:");
		toolTip.SetContentText("Max % for quantity (rags,mags,ammo,..)");
		//--
		m_InputCost.GetScript(toolTip);
		toolTip.SetTitle("Cost:");
		toolTip.SetContentText("Priority for CE (higher value should be prioritized by the spawner)");
		//--
		m_Loaded = true;
		UpdateFilter();
	}
	
	override void ShowSubMenu()
	{
		super.ShowSubMenu();
		//Do Something
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
		if (w == m_BtnLoadSelected)
		{
			if (m_ItemListBoxXML.GetSelectedRow() == -1)
			{
				GetVPPUIManager().DisplayError("No item selected!");
				return false;
			}
			GetVPPUIManager().DisplayNotification("Please wait...getting info from server");
			string typeName;
			m_ItemListBoxXML.GetItemText(m_ItemListBoxXML.GetSelectedRow(),0,typeName);
			GetRPCManager().SendRPC("RPC_XMLEditor", "GetDetails", new Param1<string>(typeName), true, null);
			return true;
		}
		
		return super.OnClick(w, x, y, button);
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
		
		if ( type == CallType.Client )
		{
			autoptr map<string,vector> result = data.param1;
			if (m_MapScreen != null)
				delete m_MapScreen;
			
			m_MapScreen = new ItemScanResultScreen();
			foreach(string name, vector pos: result)
			{
				m_MapScreen.DrawMarker(name,pos);
			}
			m_MapScreen.SetResultText("Total Scanned Objects: ["+ result.Count() +"]");
		}
	}
	
	void HandleDetails(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		ref Param2<string, ref array<ref Param3<string,string,int>>> data; //elemnt type, value, index in file
		if (!ctx.Read(data)) return;
		
	 	if ( type == CallType.Client )
		{
			ResetInputBoxes();
			m_cacheData    = data.param2;
			m_selectedType = data.param1;

			foreach(ref Param3<string,string,int> param : m_cacheData)
			{
				switch(param.param1)
				{
					case "nominal":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputNominal.SetText("NOT DEFINED!");
						m_InputNominal.Enable(false);
					}else{
						m_InputNominal.SetText(param.param2);
					}
					break;
					
					case "lifetime":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputLifetime.SetText("NOT DEFINED!");
						m_InputLifetime.Enable(false);
					}else{
						m_InputLifetime.SetText(param.param2);
					}
					break;
					
					case "restock":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputRestock.SetText("NOT DEFINED!");
						m_InputRestock.Enable(false);
					}else{
						m_InputRestock.SetText(param.param2);
					}
					break;
					
					case "min":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputMin.SetText("NOT DEFINED!");
						m_InputMin.Enable(false);
					}else{
						m_InputMin.SetText(param.param2);
					}
					break;
					
					case "quantmin":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputQuantmin.SetText("NOT DEFINED!");
						m_InputQuantmin.Enable(false);
					}else{
						m_InputQuantmin.SetText(param.param2);
					}
					break;
					
					case "quantmax":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputQuantmax.SetText("NOT DEFINED!");
						m_InputQuantmax.Enable(false);
					}else{
						m_InputQuantmax.SetText(param.param2);
					}
					break;
					
					case "cost":
					if (param.param2 == "NOT DEFINED" && param.param3 == -1){
						m_InputCost.SetText("NOT DEFINED!");
						m_InputCost.Enable(false);
					}else{
						m_InputCost.SetText(param.param2);
					}
					break;
				}
			}
			GetVPPUIManager().DisplayNotification("Successfully retrieved data from server!");
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
				GetVPPUIManager().DisplayError("Failed to save...no data to save!");
				return;
			}
			UpdateCacheData();
			GetRPCManager().SendRPC("RPC_XMLEditor", "EditElemets", new Param2<string, ref array<ref Param3<string,string,int>>>(m_selectedType,m_cacheData), true, null);
			
			m_selectedType = "";
			m_cacheData    = null;
			ResetInputBoxes();
		}
	}
	
	void RequestStats(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			string typeName;
			m_ItemListBoxXML.GetItemText(m_ItemListBoxXML.GetSelectedRow(),0,typeName);
			if (typeName == "")
			{
				GetVPPUIManager().DisplayError("You need to have an item selected from the list!");
				return;
			}
			m_MapScreen = new ItemScanResultScreen();
			GetRPCManager().SendRPC("RPC_XMLEditor", "GetScanInfo", new Param1<string>(typeName), true, null);
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
                if ((strSearch != "" && (!lowerCasedName.Contains(strSearch)))) 
                {
                    continue;
                }
                m_ItemListBoxXML.AddItem( strName, NULL, 0 );
            }
        }
	}
	
	void UpdatePreviewWidget()
	{
		if (!IsSubMenuVisible() || !m_Loaded) return;
		
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