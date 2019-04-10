class ServerSettingsUI
{
	private ref VPPAdminHud m_MainFrameClass;
	private Widget      M_MAIN_ROOT;
	private Widget      M_SUB_ROOT;
	private Widget      m_dropDown_root;

	private ref DEXMLUI m_XMLEditorClass;
	private ref InfectedHordeUI   m_InfectedHordesClass;

	private ItemPreviewWidget m_ItemPreviewXML;
	private EditBoxWidget	  m_SearchBoxXML;
	private TextListboxWidget m_ItemListBoxXML;
	private ButtonWidget      m_BtnLoadSelected;
	private ButtonWidget 	  m_XMLEditorBTN;
	private Widget       	  m_Panel_XMLEditor;
	private Widget       	  m_Panel_InfectedHordes;
	private Widget            m_Panel_SpawnPointsEditor;

	protected ref DropdownPrefab		    m_KBDropdown;
	private ref ItemPreviewInstance         m_PreviewClass;
	private ref array<ref InfoBoxInstance>  m_InfoBoxClass;

	private ref map<string,vector> m_SpawnPointsStrVector;
	private ref array<ref VPPSpawnPoint> m_MapSpawnPoints;
	private TextListboxWidget m_ListPosSP;
	private CheckBoxWidget m_ChkBoxRandomLD;
	private CheckBoxWidget m_ChkBoxStaticLD;
	private CheckBoxWidget m_ChkBoxForcedSpawns;
	private EditBoxWidget  m_InputCooldownSP;
	private ButtonWidget   m_BtnEditPoints;
	private ButtonWidget   m_BtnApplySP;
	private ButtonWidget   m_BtnGetStats;
	private TextWidget     m_TxtSPStatus;

	void ServerSettingsUI( VPPAdminHud MainFrameClass )
	{
		m_MainFrameClass = MainFrameClass;

		GetRPCManager().AddRPC( "RPC_ServerSettingsUI", "sortSpawnPoints", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_ServerSettingsUI", "sortSpawnSettings", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_ServerSettingsUI", "sortDBData", this, SingeplayerExecutionType.Server );
	}

	void ~ServerSettingsUI()
	{
		m_MainFrameClass = NULL;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.UpdatePreview);
	}

	void Init(Widget RootLayout, Widget SubRootLayout, bool HasPerm)
	{
		if (HasPerm)
		{
			M_MAIN_ROOT  = RootLayout;
			M_SUB_ROOT   = SubRootLayout;

			m_MainFrameClass.m_DraggableWindows.Insert("Panel_Settings","true");
			WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( M_SUB_ROOT,  m_MainFrameClass, "IsDraggingWindow" );

			//-=-=-=-=-=-=-=-=XML-Editor-=-=-=-=-=-=-=-=-=
			m_Panel_XMLEditor = Widget.Cast(RootLayout.FindAnyWidget( "Panel_XMLEditor" ));
			m_ItemPreviewXML  = ItemPreviewWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ItemPreviewXML" ) );
			m_XMLEditorBTN    = ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnXMLEditorApply" ) );
			m_BtnGetStats     = ButtonWidget.Cast(M_SUB_ROOT.FindAnyWidget( "BtnGetStats" ));
			m_SearchBoxXML    = EditBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "SearchBoxXML" ) );
			m_ItemListBoxXML  = TextListboxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ItemListBoxXML" ) );
			m_BtnLoadSelected = ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnLoadSelected" ) );

			m_XMLEditorClass  = new DEXMLUI(this,M_SUB_ROOT);
			//-=-=-=-=-=-=-=-=--=-Infected Hordes=-=-=-=-=-=-=-=-=-=-=--=-
			m_Panel_InfectedHordes = Widget.Cast(RootLayout.FindAnyWidget( "Panel_InfectedHordes" ));
			m_InfectedHordesClass  = new InfectedHordeUI(this,M_SUB_ROOT,m_Panel_InfectedHordes);

			//-=-=-=-=-=-=-=-=--=-Spawn Points Editor=-=-=-=-=-=-=-=-=-=-=--=-
			GetRPCManager().SendRPC("RPC_SpawnPointsManager", "GetSpawnPointsArray", NULL, true);
			m_Panel_SpawnPointsEditor = Widget.Cast(RootLayout.FindAnyWidget( "Panel_SpawnPointsEditor" ));
			m_ListPosSP          = TextListboxWidget.Cast(m_Panel_SpawnPointsEditor.FindAnyWidget( "ListPosSP" ));
			m_ChkBoxRandomLD     = CheckBoxWidget.Cast(m_Panel_SpawnPointsEditor.FindAnyWidget( "ChkBoxRandomLD" ));
			m_ChkBoxStaticLD     = CheckBoxWidget.Cast(m_Panel_SpawnPointsEditor.FindAnyWidget( "ChkBoxStaticLD" ));
			m_ChkBoxForcedSpawns = CheckBoxWidget.Cast(m_Panel_SpawnPointsEditor.FindAnyWidget( "ChkBoxForcedSpawns" ));
			m_InputCooldownSP    = EditBoxWidget.Cast(m_Panel_SpawnPointsEditor.FindAnyWidget( "InputCooldownSP" ));
			m_BtnEditPoints      = ButtonWidget.Cast(m_Panel_SpawnPointsEditor.FindAnyWidget( "BtnEditPoints" ));
			m_BtnApplySP         = ButtonWidget.Cast(m_Panel_SpawnPointsEditor.FindAnyWidget( "BtnApplySP" ));
			m_TxtSPStatus		 = TextWidget.Cast(m_Panel_SpawnPointsEditor.FindAnyWidget( "TxtSPStatus" ));
			GetRPCManager().SendRPC("RPC_SpawnPointsServer", "getConfigInfo", NULL, true);

			m_dropDown_root   = Widget.Cast(RootLayout.FindAnyWidget( "DropDownPanel" ));
			m_KBDropdown	  = new DropdownPrefab( m_dropDown_root, "Select a Config:" );
			m_KBDropdown.m_OnSelectItem.Insert( OnSelectDropDownItem );
		    m_KBDropdown.AddElement( "XML Editor" );
		    
			
			//m_KBDropdown.AddElement( "Infected Hordes" );
			//m_KBDropdown.AddElement( "Spawn Points Editor" );

		    m_PreviewClass = new ItemPreviewInstance(m_ItemPreviewXML);
		    GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdatePreview, 100, true);

		    m_InfoBoxClass = new array<ref InfoBoxInstance>;
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageNominal","Nominal:","Items of this type are spawned to this value (assuming there is enough place in the world, CE queue is not pre-occupied and restock timer reaches 0)") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageLifetime","LifeTime:","Time in seconds before this type gets deleted (assuming no one touches it)") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageRestock","Restock:","If set to 0, CE tries to respawn type in bulks, if set to positive value, it is time in seconds before ONE item of this type is spawned again") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageMin","Minimum:","If amount of items of this type reach below this value in world, they get spawned to nominal again (they follow restock logic though)") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageQuantMin","Quantity Minimum:","Min % for quantity (rags,mags,ammo,..)") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageQuantMax","Quantity Maximum:","Max % for quantity (rags,mags,ammo,..)") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageCost","Cost:","Priority for CE (higher value should be prioritized by the spawner)") );
		    /*Infected Hordes*/
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageMaxZeds","Maximum Zombies:","The maximum number of zombies in one horde") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageMinZeds","Minimum Zombies:","The minimum number of zombies in one horde") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageLifeTime","Horde LifeTime:","The time (in seconds) a horde gets to stay in one area until despawn and spawn somewhere else") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageCoolDown","Cooldown:","The time interval (in seconds) between each horde spawn. Applies only if you are spawning more than 1 horde at a time") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageMaxHordes","Maximum Zombie Hordes:","The total number of hordes on the map") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageBroadMessage","Show Notifiction","Displays a notifiction for all players on the server iforming the horde position") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageMutantZeds","Spawn Mutants:","Spawn special types of zombie models ( currently only the transparent zombies )") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageInfoPosEdit","Edit Positions:","Opens up an interactive map with markers to let you edit the locations of the hordes.") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImageItemStats","Item DataBase Statistics:","This feature allows you to see how many of the selected item are on the map! WARNING: This feature can draw-back performance as it scans the whole world map for items! hick-ups expected while scanning!") );

		    //Show XML editor by default
		    m_Panel_XMLEditor.Show(true);
		    m_XMLEditorBTN.Enable(false);
		    ChangeFilter();
		}
	}

	void HideAllSubMenus()
	{
		m_Panel_XMLEditor.Show(false);
		m_Panel_InfectedHordes.Show(false);
		m_Panel_SpawnPointsEditor.Show(false);
	}

	void OnSelectDropDownItem( int index )
	{
		HideAllSubMenus();
		switch(index)
		{
			case 0:
			m_KBDropdown.SetText( "XML Editor" );
			if (m_Panel_XMLEditor.IsVisible()) { m_Panel_XMLEditor.Show(false); }else{ m_Panel_XMLEditor.Show(true); }
			break;

			case 1:
			m_KBDropdown.SetText( "Infected Hordes" );
			m_Panel_InfectedHordes.Show(true);
			break;

			case 2:
			m_KBDropdown.SetText( "Spawn Points Editor" );
			m_Panel_SpawnPointsEditor.Show(true);
			break;
		}
		m_KBDropdown.Close();
	}

	void ChangeFilter()
	{
		TStringArray FuckedItems = new TStringArray;
		FuckedItems.Insert("Light");
		FuckedItems.Insert("ItemOptics");
		FuckedItems.Insert("SurrenderDummyItem");
		FuckedItems.Insert("TestObject");
		m_ItemListBoxXML.ClearItems();

        TStringArray cfgPaths = new TStringArray;
        cfgPaths.Insert( "CfgVehicles" );
        cfgPaths.Insert( "CfgWeapons" );
        cfgPaths.Insert( "CfgMagazines" );
        cfgPaths.Insert( "CfgNonAIVehicles" );

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

                if ( scope == 1 )
                    continue;

                string lowerCasedName = strName;
                lowerCasedName.ToLower();
                if ( GetGame().IsKindOf( lowerCasedName, "inventory_base" ) || GetGame().IsKindOf( lowerCasedName, "rifle_base" ) || GetGame().IsKindOf( lowerCasedName, "pistol_base" ) || GetGame().IsKindOf( lowerCasedName, "magazine_base" ))
                {
                	if ((strSearch != "" && (!lowerCasedName.Contains(strSearch))) || !FuckedItems.Find(strName)) 
                    {
                        continue;
                    }
                    m_ItemListBoxXML.AddItem( strName, NULL, 0 );
                }
            }
        }
	}

	void sortDBData(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		ref Param1<ref map<string,vector>> data;
		if(!ctx.Read(data)) return;

		if(type == CallType.Client)
		{
			ref CustomMapWidget m_CustomMapWidget = new CustomMapWidget("DisplayPosition",this,NULL,data.param1);
			m_XMLEditorClass.SetStatusText("Item Data Successfuly Received!");
		}
	}

	void sortSpawnSettings(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		//CUSTOMIZABLE_LOADOUTS,STATIC_LOADOUTS,FORCED_RANDOM_SPAWNS
		ref Param3<bool,bool,bool> data;
		if(!ctx.Read(data)) return;

		if(type == CallType.Client)
		{
			m_ChkBoxRandomLD.SetChecked(data.param1);
			m_ChkBoxStaticLD.SetChecked(data.param2);
			m_ChkBoxForcedSpawns.SetChecked(data.param3);
		}
	}

	//Spawn Points Editor
	void sortSpawnPoints(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		ref Param1<ref array< ref VPPSpawnPoint>> data;
		if(!ctx.Read(data)) return;

		if(type == CallType.Client)
		{
			m_MapSpawnPoints = data.param1;
			m_SpawnPointsStrVector = new map<string,vector>;
			m_ListPosSP.ClearItems();

		    for (int i = 0; i < m_MapSpawnPoints.Count(); ++i)
		    {
		    	ref VPPSpawnPoint point = m_MapSpawnPoints.Get(i);
		    	m_ListPosSP.AddItem(point.getSpawnName(), NULL, 0 );	
				m_ListPosSP.SetItem( i, g_Game.VectorToString(point.getSpawnLocation()), NULL, 1 );

				m_SpawnPointsStrVector.Insert(point.getSpawnName(),point.getSpawnLocation());
		    }
		}
	}

	void UpdateSpawnPointsPositions(ref map<string,vector> newPositions)
	{
		//Update the Textlist
		m_ListPosSP.ClearItems();
		m_MapSpawnPoints = new array<ref VPPSpawnPoint>;
		m_SpawnPointsStrVector = new map<string,vector>;

		for (int i = 0; i < newPositions.Count(); ++i)
	    {
	    	ref VPPSpawnPoint NewPoint = ;
	    	string town    = newPositions.GetKey(i);
	    	vector postown = newPositions.Get(town);

	    	string StrPos  = g_Game.VectorToString(postown);
	    	m_ListPosSP.AddItem(town, NULL, 0 );	
			m_ListPosSP.SetItem( i, StrPos, NULL, 1 );

			m_MapSpawnPoints.Insert( new VPPSpawnPoint(town,postown,600) );
			m_SpawnPointsStrVector.Insert(town,postown);
	    }
	}

	void SaveSpawnMenuSettings()
	{
		ref Param3<bool,bool,bool> oParams = new Param3<bool,bool,bool>(m_ChkBoxRandomLD.IsChecked(),m_ChkBoxStaticLD.IsChecked(),m_ChkBoxForcedSpawns.IsChecked());
	    GetRPCManager().SendRPC( "RPC_SpawnPointsServer", "setConfigInfo", oParams, true );
	}

	void SaveSpawnPointsPositions()
	{
		ref Param1<ref array<ref VPPSpawnPoint>> map_param = new Param1<ref array<ref VPPSpawnPoint>>(m_MapSpawnPoints);
	    GetRPCManager().SendRPC( "RPC_SpawnPointsManager", "UpdateSpawnPoints", map_param, true );
	}

	//Infected Hordes
	void SaveInfectedHordesPositions(ref map<string,vector> newPositions)
	{
		//This also triggers save on all modifications
		m_InfectedHordesClass.UpdatePositions(newPositions);
	}

	//XML Editor
	void UpdatePreview()
	{
		if (GetSelectedItem() != m_XMLEditorClass.GetLastSelected() && GetSelectedItem() != "" && m_Panel_XMLEditor.IsVisible())
		{
			m_PreviewClass.SetPrewViewItem(GetSelectedItem());
			m_XMLEditorClass.SetLastSelected(GetSelectedItem());
		}

		if (m_BtnGetStats != NULL && GetSelectedItem() != "")
		{
			m_BtnGetStats.Enable(true);
		}else if (m_BtnGetStats != NULL){
			m_BtnGetStats.Enable(false);
		}
	}

	//XML Items List
	string GetSelectedItem()
	{
		if (m_ItemListBoxXML == NULL) return "";
		int oRow = m_ItemListBoxXML.GetSelectedRow();
		string ItemClassName;
		if (oRow != -1)
		{
			m_ItemListBoxXML.GetItemText(oRow, 0, ItemClassName);
			return ItemClassName;
		}
		return "";
	}

	void ButtonCoolDown(ButtonWidget button)
	{
		button.Enable(true);
		button.Update();
	}
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == m_SearchBoxXML)
		{
			ChangeFilter();
			return true;
		}
		return false;
	}

	ref VPPSpawnPoint GetPointByName(string Name)
	{
		for (int x = 0; x < m_MapSpawnPoints.Count(); ++x)
	    {
	    	ref VPPSpawnPoint point = m_MapSpawnPoints.Get(x);
	    	if (point.getSpawnName() == Name)
	    	{
	    		return point;
	    	}	
	    }
	    return NULL;
	}

	bool OnClick( Widget w, int x, int y, int button )
	{
		switch(w)
		{
			case m_ChkBoxRandomLD:
			if (m_ChkBoxStaticLD.IsChecked())
			{
				m_ChkBoxStaticLD.SetChecked(false);
				return true;
			}
			break;

			case m_ChkBoxStaticLD:
			if (m_ChkBoxRandomLD.IsChecked())
			{
				m_ChkBoxRandomLD.SetChecked(false);
				return true;
			}
			break;

			case m_ListPosSP:
			string selectedTown;
			m_ListPosSP.GetItemText(m_ListPosSP.GetSelectedRow(), 0, selectedTown);
			ref VPPSpawnPoint point = GetPointByName(selectedTown);
			if (point != NULL)
			{
				m_InputCooldownSP.SetText( point.getCooldownTime().ToString() );
			}
			return true;
			break;

			case m_BtnApplySP:
			if (m_ListPosSP.GetSelectedRow() != -1)
			{
				string CoolDownText,selectedPoint;
				m_ListPosSP.GetItemText(m_ListPosSP.GetSelectedRow(), 0, selectedPoint);
				CoolDownText = m_InputCooldownSP.GetText();
				if (CoolDownText != "" && selectedPoint != "")
				{
					ref VPPSpawnPoint SpawnPoint = GetPointByName(selectedPoint);
					SpawnPoint.setCooldown( CoolDownText.ToFloat() );
				}

				SaveSpawnPointsPositions();
			} else {
				SaveSpawnPointsPositions();
			}
			SaveSpawnMenuSettings();
			m_TxtSPStatus.SetText("Changes Applied:"+g_Game.GetDate(false));
			return true;
			break;

			case m_BtnEditPoints:
			ref CustomMapWidget m_CustomMapWidget = new CustomMapWidget("SpawnPointsEditor",this,m_Panel_SpawnPointsEditor,m_SpawnPointsStrVector);
			return true;
			break;

			case m_BtnLoadSelected:
			if (GetSelectedItem() != "")
			{
				m_XMLEditorClass.GetXMLData(GetSelectedItem());
				m_XMLEditorClass.EmptyInputs("");
				m_XMLEditorClass.SetStatusText("Please Wait....Requesting Information from Server!");

				m_BtnLoadSelected.Enable(false);
				m_BtnLoadSelected.Update();
				
				m_XMLEditorBTN.Enable(true);
				m_XMLEditorBTN.Update();

				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ButtonCoolDown, 3000, false, m_BtnLoadSelected);
				return true;
			}
			break;

			case m_BtnGetStats:
			m_BtnGetStats.Enable(false);
			ref Param1<string> param_ = new Param1<string>(GetSelectedItem());
        	GetRPCManager().SendRPC( "RPC_AdminTools", "getItemCount", param_, true );
        	m_XMLEditorClass.SetStatusText("Please wait...This can take more than a minute!");
        	return true;
			break;

			case m_SearchBoxXML:
			ChangeFilter();
			return true;
			break;

			case m_XMLEditorBTN:
			m_XMLEditorClass.SetStatusText("Successfuly Applied! Back-up AND New Edit Saved in Server Profile Folder");
			
			//Call server to update new values
			ref map<int, ref Param2<string,string>> m_Data = m_XMLEditorClass.GetNewValues();
		    for (int i = 0; i < m_Data.Count(); ++i)
		    {
		    	int linePos                         = m_Data.GetKey(i);
		    	ref Param2<string,string> NewValues = m_Data.Get(linePos);

		    	Param3<int,string,string> map_param = new Param3<int,string,string>(linePos,NewValues.param1,NewValues.param2);
        		GetRPCManager().SendRPC( "RPC_SetDataByField", "SetDataByField", map_param, true );
		    }

		    m_XMLEditorBTN.Enable(false);
			m_XMLEditorBTN.Update();

			return true;
			break;
		}
		return false;
	}
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
}