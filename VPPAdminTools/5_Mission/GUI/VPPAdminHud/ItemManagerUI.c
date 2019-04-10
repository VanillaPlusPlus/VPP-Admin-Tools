class ItemManagerUI
{
	protected VPPAdminHud m_MainFrameClass;

	protected Widget M_MAIN_ROOT;
	protected Widget M_SUB_ROOT;
	protected Widget M_PANEL_ITEM_MANAGER;

	protected string m_CurrentCatagory = "All";
	protected ref array<EntityAI> m_PrewViewObjects;
	protected int m_RotationX;
	protected int m_RotationY;
	protected vector m_ItemOrientation;

	private CheckBoxWidget    m_CheckBoxSpawnOnPlayer;
	private ButtonWidget      m_ButtonSpawnInventory;
	private ButtonWidget      m_ButtonSpawnGround;
	private XComboBoxWidget   m_ComboCondition;
	private EditBoxWidget     m_InputQuantity;
	private CheckBoxWidget    m_ChkBoxOnCursor;

	private ItemPreviewWidget m_ItemPreviewWidget;
	private TextListboxWidget m_ItemListBox;
	private EditBoxWidget     m_SearchInputBox;
	private XComboBoxWidget   m_ComboCatagory;

	protected int m_LastSlot;

	private bool m_is_Active;

	void ItemManagerUI(  VPPAdminHud MainFrameClass  )
	{
		m_MainFrameClass = MainFrameClass;
	}

	void ~ItemManagerUI()
	{
		m_MainFrameClass = NULL;

	    for (int i = 0; i < m_PrewViewObjects.Count(); ++i)
	    {
	    	EntityAI obj;
	    	obj = EntityAI.Cast( m_PrewViewObjects.Get(i) );
	    	if (obj != NULL)
	    	{
	    		GetGame().ObjectDelete(obj);
	    	}
	    }
	    GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.UpdatePreview); 
	}

	void Init(Widget RootLayout, Widget SubRootLayout, bool HasPerm)
	{
		if (HasPerm)
		{
			m_is_Active      = true;

			M_MAIN_ROOT  = RootLayout;
			M_SUB_ROOT   = SubRootLayout; //Panel_ItemManager

			m_PrewViewObjects = new array<EntityAI>;

			m_MainFrameClass.m_DraggableWindows.Insert("Panel_ItemManager","true");
			WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( M_SUB_ROOT,  m_MainFrameClass, "IsDraggingWindow" );

			m_ItemPreviewWidget = ItemPreviewWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ItemPreviewWidget" ) );
			m_ItemListBox		= TextListboxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ItemListBox" ) );
			m_SearchInputBox    = EditBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "SearchInputBox" ) );
			m_ComboCatagory	    = XComboBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ComboCatagory" ) );

			m_CheckBoxSpawnOnPlayer	    = CheckBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "CheckSpnOnPlayers" ) );
			m_ButtonSpawnInventory	    = ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ButtonSpawnInventory" ) );
			m_ButtonSpawnGround	        = ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ButtonSpawnGround" ) );
			m_ComboCondition	        = XComboBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ComboCondition" ) );
			m_InputQuantity	            = EditBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "InputQuantity" ) );
			m_ChkBoxOnCursor	        = CheckBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ChkBoxOnCursor" ) );

			m_ComboCatagory.AddItem("All");			//All
			m_ComboCatagory.AddItem("Food");        //edible_base
			m_ComboCatagory.AddItem("Vehicles");    //transport
			m_ComboCatagory.AddItem("FireArms");    //weapon_base
			m_ComboCatagory.AddItem("Clothing");    //clothing_base
			m_ComboCatagory.AddItem("Structures");  //house
			m_ComboCatagory.AddItem("AI");          //dz_lightai
			ChangeFilter();

			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdatePreview, 100, true);
		}
	}

	void ShowSubRoot()
	{
		if (m_is_Active)
		{
			if (M_SUB_ROOT.IsVisible())
			{
				M_SUB_ROOT.Show(false);
			}
			else
			{
				M_SUB_ROOT.Show(true);
			}
		}
	}

	//ClassName, SpawnType, Crosshair Pos, ItemQuantity, Damage State, playerarrayGUID
	void SpawnObject(string ClassName, string SpawnType, vector CrosshairPos, float ItemQuantity, float DamageState, ref array<string> SelectedPlayers)
	{
		Param6<string,string,vector,float,float,ref array<string>> map_param;
		if (m_MainFrameClass.m_PlayerManagerUI != NULL && m_CheckBoxSpawnOnPlayer.IsChecked() && m_MainFrameClass.m_PlayerManagerUI.m_SelectedPlayers.Count() >= 1 && SelectedPlayers != NULL)
		{
			map_param = new Param6<string,string,vector,float,float,ref array<string>>(ClassName,SpawnType,CrosshairPos,ItemQuantity,DamageState,SelectedPlayers);
	    	GetRPCManager().SendRPC( "RPC_AdminTools", "SpawnObject", map_param, true );
		}
		else
		{
			map_param = new Param6<string,string,vector,float,float,ref array<string>>(ClassName,SpawnType,CrosshairPos,ItemQuantity,DamageState,NULL);
	    	GetRPCManager().SendRPC( "RPC_AdminTools", "SpawnObject", map_param, true );
		}
	}

	void UpdatePreview()
	{
		if (M_SUB_ROOT != NULL && M_SUB_ROOT.IsVisible())
		{
			int oRow = m_ItemListBox.GetSelectedRow();
			string ItemClassName;
			if (oRow != -1 && m_LastSlot != oRow && !GetGame().IsKindOf( ItemClassName, "DZ_LightAI" ))
			{
				m_ItemListBox.GetItemText(oRow, 0, ItemClassName);
				SetPrewViewItem(ItemClassName);
				m_LastSlot = oRow;
				m_ItemOrientation = Vector(0,0,0);
			}
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

	void SetPrewViewItem(string ClassName)
	{
		if (GetGame().IsKindOf( ClassName, "dz_lightai" )) return;
		bool isSpawned = false;
		EntityAI SpawnedItem;
		if (m_PrewViewObjects != NULL && m_PrewViewObjects.Count() >= 1)
		{
			for (int i = 0; i < m_PrewViewObjects.Count(); ++i)
		    {
		    	EntityAI obj = m_PrewViewObjects.Get(i);
		    	if (obj != NULL)
		    	{
		    		string Type = obj.GetType();
		    		if (Type == ClassName)
		    		{
		    			isSpawned = true;
		    			SpawnedItem = EntityAI.Cast( m_PrewViewObjects.Get(i) );
		    		}
		    	}
		    }
		}
		
	    if (isSpawned && SpawnedItem != NULL)
	    {
	    	m_ItemPreviewWidget.SetItem( SpawnedItem );
			m_ItemPreviewWidget.SetModelPosition( Vector(-5,0,0) );
			m_ItemPreviewWidget.SetModelOrientation( Vector(0,0,0) );
	    }
	    else if (!isSpawned)
	    {
			EntityAI PrewViewItem;
			PrewViewItem = EntityAI.Cast( GetGame().CreateObject( ClassName, "0 0 0", true, false ) );
			if (PrewViewItem != NULL)
			{
			    m_ItemPreviewWidget.SetItem( PrewViewItem );
				m_ItemPreviewWidget.SetModelPosition( Vector(-5,0,0) );
				m_ItemPreviewWidget.SetModelOrientation( Vector(0,0,0) );
				m_PrewViewObjects.Insert(PrewViewItem);
			}
	    }
	}

	void ChangeFilter()
	{
		m_ItemListBox.ClearItems();

        TStringArray cfgPaths = new TStringArray;
        cfgPaths.Insert( "CfgVehicles" );
        cfgPaths.Insert( "CfgWeapons" );
        cfgPaths.Insert( "CfgMagazines" );
        cfgPaths.Insert( "CfgNonAIVehicles" );

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

                if ( scope == 0 )
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
		int selectedCatagory = m_ComboCatagory.GetCurrentItem();
		switch(selectedCatagory)
		{
			case 0:
			m_CurrentCatagory = "All";
			break;

			case 1:
			m_CurrentCatagory = "edible_base";
			break;

			case 2:
			m_CurrentCatagory = "transport";
			break;

			case 3:
			m_CurrentCatagory = "weapon_base";
			break;

			case 4:
			m_CurrentCatagory = "clothing_base";
			break;

			case 5:
			m_CurrentCatagory = "house";
			break;

			case 6:
			m_CurrentCatagory = "dz_lightai";
			break;
		}
	}

	bool isActive()
	{
		return m_is_Active;
	}

	void UpdateItemRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		vector orientation;
		if (m_ItemOrientation[0] == 0 && m_ItemOrientation[1] == 0 && m_ItemOrientation[2] == 0)
		{
			orientation        = m_ItemPreviewWidget.GetModelOrientation();
			m_ItemOrientation  = m_ItemPreviewWidget.GetModelOrientation();
		}
		else
		{
			orientation = m_ItemOrientation;
		}
		
		orientation[0] = orientation[0] + (m_RotationY - mouse_y);
		orientation[1] = orientation[1] - (m_RotationX - mouse_x);

		m_ItemPreviewWidget.SetModelOrientation( orientation );

		if ( !is_dragging )
		{	
			m_ItemOrientation = orientation;
		}
	}
	
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	bool OnClick( Widget w, int x, int y, int button )
	{
		int Item_Quantity;
		int Damage_Combo;
		string StrSpwanType;
		string StrQuantity;
		ref Param6<string,string,vector,int,float,ref array<string>> map_param;

		switch(w)
		{
			case m_SearchInputBox:
			ChangeFilter();
			return true;
			break;

			case m_ComboCatagory:
			SetSearchCatagory();
			ChangeFilter();
			return true;
			break;

			case m_ButtonSpawnInventory:
			if (m_MainFrameClass.m_PlayerManagerUI != NULL)
			{
				if (m_CheckBoxSpawnOnPlayer.IsChecked() && m_MainFrameClass.m_PlayerManagerUI.m_SelectedPlayers.Count() >= 1)
				{
					Damage_Combo  = m_ComboCondition.GetCurrentItem();
					StrQuantity = m_InputQuantity.GetText();
					//ClassName, SpawnType, Crosshair Pos, ItemQuantity, Damage State
					//if (m_ChkBoxBaseAtts.IsChecked()) { StrSpwanType = "Inventory|BaseAtts"; }else{ StrSpwanType = "Inventory|NoBaseAtts"; }
					SpawnObject(GetSelectedItem(),"Inventory","0 0 0", StrQuantity.ToFloat(), m_ComboCondition.GetCurrentItem(),m_MainFrameClass.m_PlayerManagerUI.m_SelectedPlayers);
				}
				else
				{
					Damage_Combo  = m_ComboCondition.GetCurrentItem();
					StrQuantity = m_InputQuantity.GetText();
					//ClassName, SpawnType, Crosshair Pos, ItemQuantity, Damage State
					//if (m_ChkBoxBaseAtts.IsChecked()) { StrSpwanType = "Inventory|BaseAtts"; }else{ StrSpwanType = "Inventory|NoBaseAtts"; }
					SpawnObject(GetSelectedItem(),"Inventory","0 0 0", StrQuantity.ToFloat(), m_ComboCondition.GetCurrentItem(),NULL);
				}
				
			}
			return true;
			break;

			case m_ButtonSpawnGround:
			if (m_MainFrameClass.m_PlayerManagerUI != NULL)
			{
				if (m_CheckBoxSpawnOnPlayer.IsChecked() && m_MainFrameClass.m_PlayerManagerUI.m_SelectedPlayers.Count() >= 1)
				{
					Damage_Combo  = m_ComboCondition.GetCurrentItem();
					StrQuantity = m_InputQuantity.GetText();
					//ClassName, SpawnType, Crosshair Pos, ItemQuantity, Damage State
					//if (m_ChkBoxBaseAtts.IsChecked()) { StrSpwanType = "Ground|BaseAtts"; }else{ StrSpwanType = "Ground|NoBaseAtts"; }
					SpawnObject(GetSelectedItem(),"Ground","0 0 0", StrQuantity.ToFloat(), m_ComboCondition.GetCurrentItem(),m_MainFrameClass.m_PlayerManagerUI.m_SelectedPlayers);
				}
				else
				{
					Damage_Combo  = m_ComboCondition.GetCurrentItem();
					StrQuantity = m_InputQuantity.GetText();
					//if (m_ChkBoxBaseAtts.IsChecked()) { StrSpwanType = "Ground|BaseAtts"; }else{ StrSpwanType = "Ground|NoBaseAtts"; }
					//ClassName, SpawnType, Crosshair Pos, ItemQuantity, Damage State
					vector spwnpos = GetGame().GetPlayer().GetPosition();
					if (m_ChkBoxOnCursor.IsChecked()) { spwnpos = g_Game.GetCursorPos(); }
					SpawnObject(GetSelectedItem(),"Ground",spwnpos, StrQuantity.ToFloat(), m_ComboCondition.GetCurrentItem(),NULL);
				}
			}
			return true;
			break;
		}
		return false;
	}

	bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == m_SearchInputBox)
		{
			ChangeFilter();
			return true;
		}
		return false;
	}

	bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		switch(w)
		{
			case m_ItemPreviewWidget:
			GetGame().GetDragQueue().Call(this, "UpdateItemRotation");
			g_Game.GetMousePos(m_RotationX, m_RotationY);
			return true;
			break;
		}
		return false;
	}

	bool OnMouseWheel(Widget  w, int  x, int  y, int wheel)
	{
		return false;
	}
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
}

/*
const int STATE_RUINED 		 	= 4;
const int STATE_BADLY_DAMAGED 	= 3;
const int STATE_DAMAGED 	  	= 2;
const int STATE_WORN 		  	= 1;
const int STATE_PRISTINE 	  	= 0;
*/