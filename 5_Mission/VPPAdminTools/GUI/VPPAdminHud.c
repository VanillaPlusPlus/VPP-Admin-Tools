typedef Param4<string,string,string,string> VPPButtonProperties; //Permission type, display name, image, description

class VPPAdminHud extends VPPScriptedMenu
{
	private ref map<string,bool>   m_ButtonPerms;
	private ref array<ref VPPButtonProperties>  m_DefinedButtons;
	private bool m_Init;
	private ref array<ref AdminHudSubMenu> M_SUB_MENUS;
	private WrapSpacerWidget m_WrapSpacerWidget;
	private ref array<ref VPPButton> m_Buttons;

	static ref ScriptInvoker m_OnPermissionsChanged = new ScriptInvoker(); //invoker
	static ref ScriptInvoker m_OnUpdate = new ScriptInvoker(); //update queue
	
	void VPPAdminHud()
	{
		GetRPCManager().AddRPC( "RPC_VPPAdminHud", "VerifyButtonsPermission", this, SingeplayerExecutionType.Server );
		M_SUB_MENUS      = new array<ref AdminHudSubMenu>;
		m_ButtonPerms    = new map<string,bool>;
		m_DefinedButtons = {};
		
		/*Base actions: Keep here incase other external modules don't call super on DefineButtons()*/
		InsertButton("MenuPlayerManager", "Player Manager", "set:dayz_gui_vpp image:vpp_icon_players", "#VSTR_TOOLTIP_PLAYERMANAGER");
		InsertButton("MenuItemManager", "Items Spawner", "set:dayz_gui_vpp image:vpp_icon_item_manager", "#VSTR_TOOLTIP_ITEMMANAGER");
		InsertButton("MenuTeleportManager", "Teleport Tools", "set:dayz_gui_vpp image:vpp_icon_teleport_manager", "#VSTR_TOOLTIP_TELEPORTMANAGER");
		InsertButton("EspToolsMenu", "ESP Tools", "set:dayz_gui_vpp image:vpp_icon_esp", "ESP/XRay");
		InsertButton("MenuCommandsConsole", "Console", "set:dayz_gui_vpp image:vpp_icon_cmd_console", "#VSTR_TOOLTIP_COMMANDSMENU");
		InsertButton("MenuServerManager", "Server Settings", "set:dayz_gui_vpp image:vpp_icon_settings", "#VSTR_TOOLTIP_SERVERMANAGER");
		InsertButton("MenuWeatherManager", "Weather Editor", "set:dayz_gui_vpp image:vpp_icon_cloud", "#VSTR_TOOLTIP_WEATHERMANAGER");
		InsertButton("MenuObjectManager", "Object Builder", "set:dayz_gui_vpp image:vpp_icon_obj_editor", "#VSTR_TOOLTIP_OBJMANAGER");
		InsertButton("MenuBansManager", "Bans Manager", "set:dayz_gui_vpp image:vpp_icon_bans_editor", "#VSTR_TOOLTIP_BANSMANAGER");
		InsertButton("MenuPermissionsEditor", "Permission Editor", "set:dayz_gui_vpp image:vpp_icon_perms_editor", "#VSTR_TOOLTIP_PERMSEDITOR");
		InsertButton("MenuWebHooks", "Webhooks", "set:dayz_gui_vpp image:vpp_icon_webHooks", "#VSTR_TOOLTIP_WEBHOOKS");
		InsertButton("MenuXMLEditor", "XML Editor", "set:dayz_gui_vpp image:vpp_icon_xml_editor", "#VSTR_TOOLTIP_XMLEDITOR");
		DefineButtons();
		//----
		//Compile Permissions needed by buttons registered.
		array<string> perms = new array<string>;
		for(int i = 0; i < m_DefinedButtons.Count(); i++)
		{
			perms.Insert(m_DefinedButtons[i].param1);
		}
		GetRPCManager().VSendRPC("RPC_PermitManager", "VerifyButtonsPermission", new Param1<ref array<string>>(perms),true);
	}

	void ~VPPAdminHud()
	{
		if (VPPAdminHud.m_OnUpdate)
			VPPAdminHud.m_OnUpdate.Clear();
	}
	
	/*
		This gets called even when you ShowMenu() init only happens once
	*/
   	override Widget Init()
   	{
		if (!m_Init)
		{
			layoutRoot   	    = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/AdminToolBar/VPPAdminHud.layout");
	  	  	m_WrapSpacerWidget  = WrapSpacerWidget.Cast(layoutRoot.FindAnyWidget("WrapSpacerWidget"));
			m_Init = true;
			return layoutRoot;
		}
		return layoutRoot;
   	}
	
	/* 
		@DOC: external modules shall override this function to insert their custom buttons. Buttons' name will be the permission sent to server to check
		      meaning you should use the same name on server when you add your custom permission to the table using AddPermissionType();
	*/
	void DefineButtons(){}
	
	private void InsertButton(string permissionType, string displayName, string image, string desc)
	{
		m_DefinedButtons.Insert(new VPPButtonProperties(permissionType, displayName, image, desc));
	}

	private void CreateButtons()
	{
		if (!m_WrapSpacerWidget || !m_DefinedButtons)
			return;

		foreach(VPPButtonProperties data : m_DefinedButtons)
		{
			Widget w = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/AdminToolBar/VPPButton.layout", m_WrapSpacerWidget);
			m_Buttons.Insert(new VPPButton(w, data.param1, data.param2, data.param3, data.param4));
		}
	}
		
	void VerifyButtonsPermission(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref map<string,bool>> data;
		if (!ctx.Read(data)) return;
		
		if(type == CallType.Client)
        {
			m_ButtonPerms = data.param1;
			m_ButtonPerms.Copy(data.param1);
			if (!m_Buttons){
				m_Buttons = {};
				CreateButtons(); //Creates UI part of things regardless of permission(s)
			}
			VPPAdminHud.m_OnPermissionsChanged.Invoke(m_ButtonPerms);
		}
	}
	
	override void Update(float timeslice)
	{
		super.Update(timeslice);
		foreach(AdminHudSubMenu m : M_SUB_MENUS)
		{
			m.OnUpdate(timeslice);
		}
	}

	void SetWindowPriorty(AdminHudSubMenu subMenu)
	{
		int index = M_SUB_MENUS.Find(subMenu);
		if (index > -1)
		{
			//swap this item to be at first index within array
			if (index != 0)
			{
				M_SUB_MENUS.SwapItems(index, 0);
				subMenu.HideBrokenWidgets(false);
				
				for(int x = 0; x < M_SUB_MENUS.Count(); x++)
				{
					//Call to hide any broken widgets TODO: remove once DayZ fixes
					if (M_SUB_MENUS[x] != subMenu)
					{
						M_SUB_MENUS[x].HideBrokenWidgets(true);
					}
					M_SUB_MENUS[x].M_SUB_WIDGET.SetSort(M_SUB_MENUS.Count() - x);
				}
			}
		}
	}
		
	void CreateSubMenu(typename subMenuType)
	{
		if (GetSubMenuByType(subMenuType) == NULL)
		{
			AdminHudSubMenu menu = AdminHudSubMenu.Cast(subMenuType.Spawn());
			menu.OnCreate(layoutRoot);
			menu.ShowSubMenu();
			M_SUB_MENUS.Insert(menu);
		}
	}
	
	AdminHudSubMenu GetSubMenuByType(typename subMenuType)
	{
		for(int i = 0; i < M_SUB_MENUS.Count(); i++)
		{
			AdminHudSubMenu SubMenu = M_SUB_MENUS[i];
			
			if (SubMenu.ClassName().ToType() == subMenuType)
			{
				return SubMenu;
			}
		}
		return NULL;
	}
};