class MenuItemManager extends AdminHudSubMenu
{
	//Catalog (left panel)
	private EditBoxWidget     		 m_SearchInputBox;
	private TextListboxWidget 		 m_ItemListBox;
	private TextWidget        		 m_txtItemCount;
	private ImageWidget       		 m_SelItemIcon;
	private TextWidget        		 m_SelItemName;
	private TextWidget        		 m_SelItemClass;
	private ButtonWidget      		 m_BtnFavorite;
	private ImageWidget       		 m_ImgFavorite;
	private ButtonWidget      		 m_BtnAddToPreset;
	private ButtonWidget      		 m_BtnQuickSpawn;
	private ButtonWidget             m_btnRefresh;
	protected ref VPPDropDownMenu 	 m_CategoryDropDown;

	//Workbench tabs (right panel)
	private ButtonWidget      		 m_BtnTabSpawn;
	private ButtonWidget      		 m_BtnTabPresets;
	private TextWidget        		 m_TabSpawnLbl;
	private TextWidget        		 m_TabPresetsLbl;
	private TextWidget        		 m_TxtActivePreset;
	private Widget            		 m_SpawnRoot;
	private Widget            		 m_PresetsRoot;
	private bool              		 m_PresetsTabActive;

	//SPAWN tab
	private ItemPreviewWidget 		 m_ItemPreview;
	private CheckBoxWidget           m_chkBoxPreview;
	private EditBoxWidget          	 m_InputQuantity;
	private ButtonWidget      		 m_BtnQtyMinus;
	private ButtonWidget      		 m_BtnQtyPlus;
	private ButtonWidget      		 m_BtnQtyMax;
	private CheckBoxWidget           m_ChkOnSelectedPlayers;
	private CheckBoxWidget 			 m_ChkUseCESetup;
	private ButtonWidget      		 m_BtnSpawnItem;
	protected ref VPPDropDownMenu 	 m_ConditionDropDown;
	protected ref VPPDropDownMenu 	 m_PlacementDropDown;

	//PRESETS tab
	private GridSpacerWidget         m_PresetsGrid;
	private ScrollWidget 		 	 m_PresetsScroll;
	private TextWidget        		 m_txtPresetCount;
	private TextWidget        		 m_TxtContentsPreset;
	private GridSpacerWidget         m_ParentGrid;
	private ScrollWidget 		 	 m_ScrollerItemList;
	private ButtonWidget             m_BtnSaveChanges;
	private ButtonWidget             m_BtnCreatePreset;
	private ButtonWidget             m_BtnDeletePreset;
	protected ref VPPDropDownMenu 	 m_PresetPlacementDropDown;

	private ref PopUpCreatePreset  	 m_PopUpPresetCreate;
	private ref Widget           	 m_PopUpPresetCreateWidget;
	private Widget 					 m_Main;

	//3D preview state
	private EntityAI 	  	  		 m_PreviewObject;
	private int 	 		  		 m_RotationX;
	private int 	 		  		 m_RotationY;
	private vector   		  		 m_ItemOrientation;

	//Data
	private ref CustomGridSpacer 	 m_LastGrid;
	private ref array<ref PresetItemEntry>  m_Entries;
	private ref array<ref CustomGridSpacer> m_DataGrids;
	private ref array<ref VPPPresetEntry>   m_PresetEntries;
	private ref array<ref PresetItemData>   m_PresetItemData; //holds all presets items from server
	private ref PresetItemData              m_CurrentPresetData;
	private int                             m_ActivePresetIndex = -1;
	private ref ItemManagerClientPrefs      m_Prefs;

	//Catalog filtering: kind keys + matching dropdown labels
	private const ref array<string> m_CatagoryKinds  = {"All", "__favorites", "__recents", "edible_base", "transport", "weapon_base", "clothing_base", "house", "dz_lightai"};
	private const ref array<string> m_CatagoryLabels = {"All Items", "Favorites", "Recent", "Food & Drink", "Vehicles", "Weapons", "Clothing", "Structures", "AI & Animals"};
	private const ref array<string> m_ConditionLabels = {"#VSTR_LBL_PRISTINE", "#VSTR_LBL_WORN", "#VSTR_LBL_DAMAGED", "#VSTR_LBL_BADLY_DAMAGED", "#VSTR_LBL_RUIEND"};
	private const ref array<string> m_PlacementLabels = {"#VSTR_LBL_IN_INENTORY", "#VSTR_LBL_ON_GROUND", "#VSTR_LBL_AT_CROSSHAIRS"};

	private string m_CurrentCatagory;
	private string m_searchBoxStr;
	private const float m_FilterUpdate = 0.35;
	private float  m_FilterUpdateCurTick;
	private bool   m_loaded;
	private int    prevRow = -2;

	void MenuItemManager()
	{
		m_DataGrids 	 = new array<ref CustomGridSpacer>;
		m_Entries   	 = new array<ref PresetItemEntry>;
		m_PresetEntries  = new array<ref VPPPresetEntry>;
		m_PresetItemData = new array<ref PresetItemData>;

		/* RPCs */
		GetRPCManager().AddRPC("RPC_MenuItemManager", "HandleData", this, SingleplayerExecutionType.Client);
		//-------
	}

	void ~MenuItemManager()
	{
		if (m_PreviewObject != null)
		GetGame().ObjectDelete(m_PreviewObject);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.PollCatalogSelection);
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);

		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuItemManager);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast(M_SUB_WIDGET.FindAnyWidget("Header"));
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );

		m_Main		  	 = M_SUB_WIDGET.FindAnyWidget( "Main");

		//Catalog
		m_SearchInputBox = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SearchInputBox") );
		m_ItemListBox    = TextListboxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemListBox") );
		m_txtItemCount   = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "txtItemCount") );
		m_SelItemIcon    = ImageWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SelItemIcon") );
		m_SelItemName    = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SelItemName") );
		m_SelItemClass   = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SelItemClass") );
		m_BtnFavorite    = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnFavorite") );
		m_ImgFavorite    = ImageWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ImgFavorite") );
		m_BtnAddToPreset = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnAddToPreset") );
		m_BtnQuickSpawn  = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnQuickSpawn") );
		m_btnRefresh 	 = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "btnRefresh") );

		//Tabs
		m_BtnTabSpawn    = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnTabSpawn") );
		m_BtnTabPresets  = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnTabPresets") );
		m_TabSpawnLbl    = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TabSpawnLbl") );
		m_TabPresetsLbl  = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TabPresetsLbl") );
		m_TxtActivePreset = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtActivePreset") );
		m_SpawnRoot      = M_SUB_WIDGET.FindAnyWidget( "SpawnRoot");
		m_PresetsRoot    = M_SUB_WIDGET.FindAnyWidget( "PresetsRoot");

		//SPAWN tab
		m_ItemPreview    = ItemPreviewWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemPreview") );
		m_chkBoxPreview  = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "chkBoxPreview") );
		m_chkBoxPreview.SetChecked(true);
	 	m_InputQuantity  = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputQuantity") );
		m_BtnQtyMinus    = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnQtyMinus") );
		m_BtnQtyPlus     = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnQtyPlus") );
		m_BtnQtyMax      = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnQtyMax") );
		m_ChkOnSelectedPlayers = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ChkOnSelectedPlayers") );
		m_ChkUseCESetup	 = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ChkUseCESetup") );
		m_BtnSpawnItem   = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnSpawnItem") );

		//PRESETS tab
		m_PresetsGrid    = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "PresetsGrid") );
		m_PresetsScroll  = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "PresetsScroll") );
		m_txtPresetCount = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "txtPresetCount") );
		m_TxtContentsPreset = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "TxtContentsPreset") );
		m_ParentGrid     = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ParentGrid"));
		m_ScrollerItemList = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ScrollerItemList") );
		m_BtnSaveChanges = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnSaveChanges") );
		m_BtnCreatePreset = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnCreatePreset") );
		m_BtnDeletePreset = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnDeletePreset") );

		//Dropdowns (design-system VPPDropDownMenu everywhere; no XComboBox)
		m_CategoryDropDown = new VPPDropDownMenu( M_SUB_WIDGET.FindAnyWidget("CategoryDropDownPanel"), m_CatagoryLabels[0] );
		foreach(string catLabel : m_CatagoryLabels){
			m_CategoryDropDown.AddElement(catLabel);
		}
		m_CategoryDropDown.m_OnSelectItem.Insert( OnSelectCategory );

		m_ConditionDropDown = new VPPDropDownMenu( M_SUB_WIDGET.FindAnyWidget("ConditionDropDownPanel"), m_ConditionLabels[0] );
		foreach(string condLabel : m_ConditionLabels){
			m_ConditionDropDown.AddElement(condLabel);
		}
		m_ConditionDropDown.SetIndex(0);
		m_ConditionDropDown.m_OnSelectItem.Insert( OnSelectCondition );

		m_PlacementDropDown = new VPPDropDownMenu( M_SUB_WIDGET.FindAnyWidget("PlacementDropDownPanel"), m_PlacementLabels[0] );
		foreach(string plcLabel : m_PlacementLabels){
			m_PlacementDropDown.AddElement(plcLabel);
		}
		m_PlacementDropDown.SetIndex(0);
		m_PlacementDropDown.m_OnSelectItem.Insert( OnSelectPlacement );

		m_PresetPlacementDropDown = new VPPDropDownMenu( M_SUB_WIDGET.FindAnyWidget("PresetPlacementDropDownPanel"), m_PlacementLabels[PlacementTypes.ON_GROUND] );
		foreach(string pplcLabel : m_PlacementLabels){
			m_PresetPlacementDropDown.AddElement(pplcLabel);
		}
		m_PresetPlacementDropDown.SetIndex(PlacementTypes.ON_GROUND);
		m_PresetPlacementDropDown.m_OnSelectItem.Insert( OnSelectPresetPlacement );

		//Client prefs (favorites + recents)
		m_Prefs = ItemManagerClientPrefs.Load();

		m_CurrentCatagory = m_CatagoryKinds[0];
		UpdateFilter();
		SetPanelTab(false);
		UpdateSelectedStrip(-1);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.PollCatalogSelection, 100, true);

		GetRPCManager().VSendRPC("RPC_VPPItemManager", "GetData", null, true, null);
		m_loaded = true;
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() && !m_loaded) return; //Update cancels if sub menu is not visible.

		//Debounced search filter
		m_FilterUpdateCurTick += timeslice;
		if (m_FilterUpdateCurTick >= m_FilterUpdate)
		{
			string newSrch = m_SearchInputBox.GetText();
			if (newSrch != m_searchBoxStr)
			{
				m_searchBoxStr = newSrch;
				UpdateFilter();
			}
			m_FilterUpdateCurTick = 0.0;
		}

		bool hasSelection = (m_ItemListBox.GetSelectedRow() != -1);
		bool hasPreset    = (m_CurrentPresetData != null);

		m_BtnQuickSpawn.Enable(hasSelection);
		m_BtnSpawnItem.Enable(hasSelection);
		m_BtnFavorite.Enable(hasSelection);
		m_BtnAddToPreset.Enable(hasSelection && hasPreset);
		m_BtnSaveChanges.Enable(hasPreset);
		m_BtnDeletePreset.Enable(hasPreset);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case m_BtnTabSpawn:
			SetPanelTab(false);
			break;

			case m_BtnTabPresets:
			SetPanelTab(true);
			break;

			case m_BtnQtyMinus:
			NudgeQuantity(-1);
			break;

			case m_BtnQtyPlus:
			NudgeQuantity(1);
			break;

			case m_BtnQtyMax:
			m_InputQuantity.SetText("MAX");
			break;

			case m_BtnSpawnItem:
			case m_BtnQuickSpawn:
			RequestSpawnSelected();
			break;

			case m_BtnAddToPreset:
			AddSelectedToPreset();
			break;

			case m_BtnFavorite:
			ToggleFavoriteSelected();
			break;

			case m_BtnSaveChanges:
			if (m_CurrentPresetData != null)
				GetRPCManager().VSendRPC("RPC_VPPItemManager", "EditPreset", new Param1<ref PresetItemData>(m_CurrentPresetData), true, null);
			break;

			case m_BtnDeletePreset:
			if (m_CurrentPresetData != null)
				DeletePreset(m_CurrentPresetData.GetPresetName());
			else
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_NOPRESET");
			break;

			case m_BtnCreatePreset:
			if (m_PopUpPresetCreateWidget != null && m_PopUpPresetCreate != null)
			{
				m_PopUpPresetCreateWidget.Unlink();
				delete m_PopUpPresetCreate;
			}
			m_PopUpPresetCreateWidget = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.PopUpCreatePreset, m_Main);
			m_PopUpPresetCreateWidget.GetScript(m_PopUpPresetCreate);
			m_PopUpPresetCreate.Init(this);
			break;

			case m_btnRefresh:
			SetActivePreset(-1);
			GetRPCManager().VSendRPC("RPC_VPPItemManager", "GetData", null, true, null);
			break;

			case m_chkBoxPreview:
			prevRow = -2; //force preview + strip refresh on next poll
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
			GetMousePos(m_RotationX, m_RotationY);
			return true;
		}
		return false;
	}

	//Double-click stays as a power shortcut: spawn / Ctrl = add to active preset
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (w == m_ItemListBox)
		{
			if(button == MouseState.LEFT)
			{
				if (g_Game.IsLeftCtrlDown())
					AddSelectedToPreset();
				else
					RequestSpawnSelected();
			}
			return true;
		}
		return super.OnDoubleClick(w, x, y, button);
	}

	/*
		Switches the right panel between the SPAWN workbench and the
		PRESETS library/contents view
	*/
	void SetPanelTab(bool showPresets)
	{
		m_PresetsTabActive = showPresets;
		m_SpawnRoot.Show(!showPresets);
		m_PresetsRoot.Show(showPresets);
		m_TxtActivePreset.Show(showPresets); //active-preset badge only makes sense on the PRESETS tab

		if (showPresets)
		{
			m_TabSpawnLbl.SetColor(ARGB(255, 154, 160, 166));   //text-secondary
			m_TabPresetsLbl.SetColor(ARGB(255, 232, 234, 237)); //text-primary
		}
		else
		{
			m_TabSpawnLbl.SetColor(ARGB(255, 232, 234, 237));
			m_TabPresetsLbl.SetColor(ARGB(255, 154, 160, 166));
		}
	}

	/*
	\ Dropdown callbacks
	*/
	void OnSelectCategory(int index)
	{
		if (index < 0 || index >= m_CatagoryKinds.Count()) return;

		m_CategoryDropDown.SetText(m_CatagoryLabels[index]);
		m_CategoryDropDown.SetIndex(index);
		m_CategoryDropDown.Close();
		m_CurrentCatagory = m_CatagoryKinds[index];
		UpdateFilter();
	}

	void OnSelectCondition(int index)
	{
		if (index < 0 || index >= m_ConditionLabels.Count()) return;

		m_ConditionDropDown.SetText(m_ConditionLabels[index]);
		m_ConditionDropDown.SetIndex(index);
		m_ConditionDropDown.Close();
	}

	void OnSelectPlacement(int index)
	{
		if (index < 0 || index >= m_PlacementLabels.Count()) return;

		m_PlacementDropDown.SetText(m_PlacementLabels[index]);
		m_PlacementDropDown.SetIndex(index);
		m_PlacementDropDown.Close();
	}

	void OnSelectPresetPlacement(int index)
	{
		if (index < 0 || index >= m_PlacementLabels.Count()) return;

		m_PresetPlacementDropDown.SetText(m_PlacementLabels[index]);
		m_PresetPlacementDropDown.SetIndex(index);
		m_PresetPlacementDropDown.Close();
	}
	/*
	\------------
	*/

	void SaveNewPreset(string presetName)
	{
		GetRPCManager().VSendRPC("RPC_VPPItemManager", "AddPreset", new Param1<string>(presetName), true, null);
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

	void DeletePreset(string presetName)
	{
		GetRPCManager().VSendRPC("RPC_VPPItemManager", "DeletePreset", new Param1<string>(presetName), true, null);
	}

	/*
		Quantity stepper: [-] value [+] MAX
		"MAX" (or empty) resolves to the item's maximum at spawn time.
	*/
	void NudgeQuantity(int direction)
	{
		float maxQuantity = ComputeMaxQuantity();
		string txtQuant = m_InputQuantity.GetText();
		txtQuant.ToLower();

		float current;
		if (txtQuant == "max" || txtQuant == string.Empty)
			current = maxQuantity;
		else
			current = m_InputQuantity.GetText().ToFloat();

		current += direction;
		if (current < 0) current = 0;
		if (maxQuantity > 0 && current > maxQuantity) current = maxQuantity;

		int rounded = Math.Round(current);
		m_InputQuantity.SetText(rounded.ToString());
	}

	private float ComputeMaxQuantity()
	{
		ItemBase ib = ItemBase.Cast(m_PreviewObject);
		if (!ib) return 0;

		if (ib.IsMagazine())
			return ib.ConfigGetInt("count");

		return ib.GetQuantityMax();
	}

	/*
		Spawns the currently selected catalog item with the SPAWN tab options
	*/
	void RequestSpawnSelected()
	{
		string typeName = GetSelectedItem();
		if (typeName == "")
		{
			GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_SPAWN_PRESET");
			return;
		}

		ItemSpawnParams params = BuildSpawnParams(typeName);
		if (params == null) return;

		GetRPCManager().VSendRPC("RPC_VPPItemManager", "SpawnItem", new Param1<ref ItemSpawnParams>(params), true, null);

		m_Prefs.RecordRecent(typeName);
		if (m_CurrentCatagory == "__recents")
			UpdateFilter();
	}

	void SpawnPresetByName(string presetName)
	{
		//presets use their own SPAWN IN choice from the PRESETS tab
		ItemSpawnParams params = BuildSpawnParams(presetName, m_PresetPlacementDropDown.GetIndex());
		if (params == null) return;

		GetRPCManager().VSendRPC("RPC_VPPItemManager", "RemoteSpawnPreset", new Param1<ref ItemSpawnParams>(params), true, null);
	}

	/*
		Shared spawn options (quantity, condition, placement, targets, CE)
		used by single-item spawns and preset spawns alike
	*/
	private ItemSpawnParams BuildSpawnParams(string targetName, int placementOverride = -1)
	{
		ItemBase ib = ItemBase.Cast(m_PreviewObject);

		float quantity = m_InputQuantity.GetText().ToFloat();

		string txtQuant = m_InputQuantity.GetText();
		txtQuant.ToLower();

		if (txtQuant == "max" || txtQuant == string.Empty)
		{
			if (ib && ib.IsMagazine()){
				quantity = ib.ConfigGetInt("count");
			}else if (ib){
				quantity = ib.GetQuantityMax();
			}
		}

		//Handles mags, ammo etc (clamps it between 0..1)
		if (ib)
		{
			if (ib.HasQuantity())
			{
				if (ib.IsMagazine())
				{
					quantity = Math.Clamp((quantity - 0.0) / (ib.ConfigGetInt("count") - 0.0), 0, 1);
				}else{
					quantity = Math.Clamp((quantity - ib.GetQuantityMin()) / (ib.GetQuantityMax() - ib.GetQuantityMin()), 0, 1);
				}
			}

			//Energy component based items
			if (ib.HasComponent(COMP_TYPE_ENERGY_MANAGER))//more direct access for speed
			{
				ComponentEnergyManager comp = ib.GetCompEM();
				if (comp && (comp.GetEnergyMaxPristine() || comp.GetEnergyAtSpawn()))
				{
					if (txtQuant == "max" || txtQuant == string.Empty)
						quantity = comp.GetEnergyAtSpawn(); //uses default config.cpp entry value "energyAtSpawn"
				}
			}
		}

		int condition = m_ConditionDropDown.GetIndex();
		int placementType = m_PlacementDropDown.GetIndex();
		if (placementOverride > -1)
			placementType = placementOverride;

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
			MenuPlayerManager pManager = MenuPlayerManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPlayerManager));
			if (pManager == null)
			{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_SPAWN_PRESET");
				return null;
			}

			trgIDs = pManager.GetSelectedPlayersIDs();
			if (trgIDs.Count() <= 0)
			{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_SPAWN_PRESET_NOPLAYER");
				return null;
			}
		}

		return new ItemSpawnParams(targetName, pos, quantity, condition, placementType, trgIDs, m_ChkUseCESetup.IsChecked());
	}

	void AddSelectedToPreset()
	{
		string typeName = GetSelectedItem();
		if (typeName == "") return;

		if (m_CurrentPresetData == null)
		{
			GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_NOTSELECTPRESET");
			return;
		}

		m_CurrentPresetData.AddItem(typeName, false);
		ReloadPresetData();
	}

	void ToggleFavoriteSelected()
	{
		string typeName = GetSelectedItem();
		if (typeName == "") return;

		m_Prefs.ToggleFavorite(typeName);
		UpdateFavoriteIcon(typeName);

		if (m_CurrentCatagory == "__favorites")
			UpdateFilter();
	}

	void ReloadPresetData()
	{
		ClearEntriesList();
		if (m_CurrentPresetData != null)
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
			//keep the active preset across refreshes by name
			string activeName = "";
			if (m_CurrentPresetData != null)
				activeName = m_CurrentPresetData.GetPresetName();

			m_PresetItemData = data.param1;

			int newActive = -1;
			for (int i = 0; i < m_PresetItemData.Count(); ++i)
			{
				if (activeName != "" && m_PresetItemData[i].GetPresetName() == activeName)
					newActive = i;
			}
			SetActivePreset(newActive);
		}
	}
	/*
	\------------
	*/

	/*
		Rebuilds the PRESETS tab library rows and the active-preset state
	*/
	void SetActivePreset(int index)
	{
		if (index < -1 || index >= m_PresetItemData.Count())
			index = -1;

		m_ActivePresetIndex = index;
		if (index == -1)
			m_CurrentPresetData = null;
		else
			m_CurrentPresetData = m_PresetItemData[index];

		RebuildPresetLibrary();

		if (m_CurrentPresetData != null)
		{
			m_TxtActivePreset.SetText(m_CurrentPresetData.GetPresetName());
			m_TxtContentsPreset.SetText(m_CurrentPresetData.GetPresetName());
		}
		else
		{
			m_TxtActivePreset.SetText("No preset");
			m_TxtContentsPreset.SetText("Select a preset");
		}

		ReloadPresetData();
	}

	private void RebuildPresetLibrary()
	{
		m_PresetEntries.Clear();

		for (int i = 0; i < m_PresetItemData.Count(); ++i)
		{
			m_PresetEntries.Insert(new VPPPresetEntry(m_PresetsGrid, this, i, m_PresetItemData[i].GetPresetName(), i == m_ActivePresetIndex));
		}

		m_txtPresetCount.SetText(string.Format("%1 presets", m_PresetItemData.Count().ToString()));
		m_PresetsGrid.Update();
		m_PresetsScroll.Update();
	}

	/*
	\ VPPPresetEntry row callbacks
	*/
	void OnPresetEntrySelect(int index)
	{
		SetActivePreset(index);
	}

	void OnPresetEntrySpawn(int index)
	{
		if (index < 0 || index >= m_PresetItemData.Count()) return;
		SpawnPresetByName(m_PresetItemData[index].GetPresetName());
	}

	void OnPresetEntryDelete(int index)
	{
		if (index < 0 || index >= m_PresetItemData.Count()) return;
		DeletePreset(m_PresetItemData[index].GetPresetName());
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

	void AddEntry(PresetItemData item)
	{
		if(m_LastGrid.GetContentCount() == 100)
		{
			m_LastGrid = new CustomGridSpacer(m_ParentGrid);
		 	m_DataGrids.Insert(m_LastGrid);
		}

		if(m_LastGrid.GetContentCount() < 100)
		{
			PresetItemEntry entry;
			//Add Children
			array<string> childItems = item.GetItemTypes();
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
		m_Entries.Clear();
	}

	/*
		Polls catalog selection: refreshes the 3D preview + selected-item strip
	*/
	void PollCatalogSelection()
	{
		if (!IsSubMenuVisible() && !m_loaded) return;

		int oRow = m_ItemListBox.GetSelectedRow();
		if (oRow == prevRow) return;

		prevRow = oRow;
		UpdateSelectedStrip(oRow);
		UpdatePreview(oRow);
	}

	private void UpdatePreview(int oRow)
	{
		if (m_PreviewObject != null)
		{
			m_ItemPreview.SetItem(null);
			GetGame().ObjectDelete(m_PreviewObject);
		}

		string itemClassName;
		if (oRow == -1)
		{
			m_ItemPreview.Show(false);
			return;
		}

		m_ItemListBox.GetItemText(oRow, 0, itemClassName);
		string lowerName = itemClassName;
		lowerName.ToLower();
		if (itemClassName == "" || GetGame().IsKindOf( lowerName, "dz_lightai" ))
		{
			m_ItemPreview.Show(false);
			return;
		}

		m_PreviewObject = EntityAI.Cast(GetGame().CreateObject(itemClassName,vector.Zero,true,false,false));
		if (m_PreviewObject != null && m_chkBoxPreview.IsChecked())
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
	}

	private void UpdateSelectedStrip(int oRow)
	{
		if (oRow == -1)
		{
			m_SelItemName.SetText("No item selected");
			m_SelItemClass.SetText("Pick an item from the list above");
			m_SelItemIcon.LoadImageFile(0, "set:vpp_icons image:package");
			m_SelItemIcon.SetColor(ARGB(255, 92, 97, 102)); //text-muted
			m_ImgFavorite.SetColor(ARGB(255, 92, 97, 102));
			return;
		}

		string className;
		string displayName;
		m_ItemListBox.GetItemText(oRow, 0, className);
		m_ItemListBox.GetItemText(oRow, 1, displayName);

		if (displayName == "")
			displayName = className;

		m_SelItemName.SetText(displayName);
		m_SelItemClass.SetText(className);
		m_SelItemIcon.LoadImageFile(0, GetTypeIconFor(className));
		m_SelItemIcon.SetColor(ARGB(255, 154, 160, 166)); //text-secondary
		UpdateFavoriteIcon(className);
	}

	private void UpdateFavoriteIcon(string className)
	{
		if (m_Prefs.IsFavorite(className))
			m_ImgFavorite.SetColor(ARGB(255, 232, 163, 61));  //accent-orange
		else
			m_ImgFavorite.SetColor(ARGB(255, 154, 160, 166)); //text-secondary
	}

	private string GetTypeIconFor(string className)
	{
		string lowerName = className;
		lowerName.ToLower();

		if (GetGame().IsKindOf(lowerName, "weapon_base"))   return "set:vpp_icons image:sword";
		if (GetGame().IsKindOf(lowerName, "clothing_base")) return "set:vpp_icons image:shirt";
		if (GetGame().IsKindOf(lowerName, "edible_base"))   return "set:vpp_icons image:apple";
		if (GetGame().IsKindOf(lowerName, "transport"))     return "set:vpp_icons image:car";
		if (GetGame().IsKindOf(lowerName, "house"))         return "set:vpp_icons image:house";
		if (GetGame().IsKindOf(lowerName, "dz_lightai"))    return "set:vpp_icons image:paw_print";
		return "set:vpp_icons image:package";
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

	/*
		Rebuilds the catalog list: category (incl. Favorites / Recent) + search,
		classname column + config display-name column
	*/
	void UpdateFilter()
	{
		m_ItemListBox.ClearItems();
		prevRow = -2;

		string strSearch = m_SearchInputBox.GetText();
		strSearch.ToLower();

		int count;
		if (m_CurrentCatagory == "__favorites")
		{
			count = FillFromList(m_Prefs.Favorites, strSearch);
		}
		else if (m_CurrentCatagory == "__recents")
		{
			count = FillFromList(m_Prefs.Recents, strSearch);
		}
		else
		{
			count = FillFromConfig(strSearch);
		}

		m_txtItemCount.SetText(count.ToString());
	}

	private int FillFromConfig(string strSearch)
	{
		TStringArray cfgPaths = new TStringArray;
		cfgPaths.Insert( "CfgVehicles" );
		cfgPaths.Insert( "CfgWeapons" );
		cfgPaths.Insert( "CfgMagazines" );
		/*cfgPaths.Insert( "CfgNonAIVehicles" );*/

		int count = 0;
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
					if (AddCatalogRow(Config_Path, strName, strSearch))
						count++;
				}
			}
		}
		return count;
	}

	private int FillFromList(array<string> typeNames, string strSearch)
	{
		int count = 0;
		foreach(string typeName : typeNames)
		{
			string cfgPath = FindConfigPathFor(typeName);
			if (cfgPath == "") continue; //mod removed / invalid classname

			if (AddCatalogRow(cfgPath, typeName, strSearch))
				count++;
		}
		return count;
	}

	//returns true when the row passed the search filter and was added
	private bool AddCatalogRow(string cfgPath, string className, string strSearch)
	{
		string displayName = GetGame().ConfigGetTextOut(cfgPath + " " + className + " displayName");
		if (GetGame().FormatRawConfigStringKeys(displayName))
			displayName = Widget.TranslateString(displayName); //resolve $STR_ keys so search matches what's shown

		if (strSearch != "")
		{
			string lowerClass = className;
			lowerClass.ToLower();
			string lowerDisplay = displayName;
			lowerDisplay.ToLower();

			if (!lowerClass.Contains(strSearch) && !lowerDisplay.Contains(strSearch))
				return false;
		}

		int row = m_ItemListBox.AddItem( className, NULL, 0 );
		if (displayName != "")
			m_ItemListBox.SetItem( row, displayName, NULL, 1 );

		return true;
	}

	private string FindConfigPathFor(string className)
	{
		if (GetGame().ConfigIsExisting("CfgVehicles " + className))  return "CfgVehicles";
		if (GetGame().ConfigIsExisting("CfgWeapons " + className))   return "CfgWeapons";
		if (GetGame().ConfigIsExisting("CfgMagazines " + className)) return "CfgMagazines";
		return "";
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
