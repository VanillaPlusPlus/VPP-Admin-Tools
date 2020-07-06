class VPPButton
{
	ButtonWidget Button;
	string 	     ButtonName;
	string 	     ButtonIcon;
	string       ButtonDescription;
	private      Widget m_ButtonWidget; 
	
	void VPPButton(Widget btn, string name, string icon, string description)
	{
		m_ButtonWidget    = btn;
		Button            = ButtonWidget.Cast(btn.FindAnyWidget("Button"));
		ButtonName        = name;
		ButtonIcon        = icon;
		ButtonDescription = description;
		ImageWidget.Cast(btn.FindAnyWidget("Image")).LoadImageFile(0, icon);
		
		autoptr ToolTipHandler toolTip;
		Button.GetScript(toolTip);
		toolTip.SetTitle(name+":");
		toolTip.SetContentText(description);
		toolTip.SetLeftSide(true);
	}
	
	void ~VPPButton()
	{
		if (m_ButtonWidget != null)
			m_ButtonWidget.Unlink();
	}
}

class VPPAdminHud extends VPPScriptedMenu
{
	private ref map<string,bool>   m_ButtonPerms;
	private ref array<ref Param3<string,string,string>>  m_DefinedButtons;
	private bool m_Init;
	private ref array<ref AdminHudSubMenu> M_SUB_MENUS;
	private WrapSpacerWidget m_WrapSpacerWidget;
	private ref array<ref VPPButton> m_Buttons;
	
	void VPPAdminHud()
	{
		GetRPCManager().AddRPC( "RPC_VPPAdminHud", "VerifyButtonsPermission", this, SingeplayerExecutionType.Server );
		M_SUB_MENUS      = new array<ref AdminHudSubMenu>;
		m_ButtonPerms    = new map<string,bool>;
		m_DefinedButtons = new ref array<ref Param3<string,string,string>>;
		m_Buttons        = new array<ref VPPButton>;
		
		/*Base actions: Keep here incase other external modules don't call super on DefineButtons()*/
		InsertButton("MenuPlayerManager" , "set:dayz_gui_vpp image:vpp_icon_players", "#VSTR_TOOLTIP_PLAYERMANAGER");
		InsertButton("MenuItemManager" , "set:dayz_gui_vpp image:vpp_icon_item_manager", "#VSTR_TOOLTIP_ITEMMANAGER");
		InsertButton("MenuTeleportManager" , "set:dayz_gui_vpp image:vpp_icon_teleport_manager", "#VSTR_TOOLTIP_TELEPORTMANAGER");
		InsertButton("EspToolsMenu" , "set:dayz_gui_vpp image:vpp_icon_esp", "ESP/XRay");
		InsertButton("MenuCommandsConsole" , "set:dayz_gui_vpp image:vpp_icon_cmd_console", "#VSTR_TOOLTIP_COMMANDSMENU");
		InsertButton("MenuServerManager" , "set:dayz_gui_vpp image:vpp_icon_settings", "#VSTR_TOOLTIP_SERVERMANAGER");
		InsertButton("MenuWeatherManager" , "set:dayz_gui_vpp image:vpp_icon_cloud", "#VSTR_TOOLTIP_WEATHERMANAGER");
		InsertButton("MenuObjectManager" , "set:dayz_gui_vpp image:vpp_icon_obj_editor", "#VSTR_TOOLTIP_OBJMANAGER");
		InsertButton("MenuBansManager" , "set:dayz_gui_vpp image:vpp_icon_bans_editor", "#VSTR_TOOLTIP_BANSMANAGER");
		InsertButton("MenuPermissionsEditor" , "set:dayz_gui_vpp image:vpp_icon_perms_editor", "#VSTR_TOOLTIP_PERMSEDITOR");
		InsertButton("MenuWebHooks" , "set:dayz_gui_vpp image:vpp_icon_webHooks", "#VSTR_TOOLTIP_WEBHOOKS");
		InsertButton("MenuXMLEditor" , "set:dayz_gui_vpp image:vpp_icon_xml_editor", "#VSTR_TOOLTIP_XMLEDITOR");
		//----
		DefineButtons();
		//Compile Permissions needed by buttons registred.
		autoptr array<string> perms = new array<string>;
		for(int i = 0; i < m_DefinedButtons.Count(); i++)
		{
			perms.Insert(m_DefinedButtons.Get(i).param1);
		}
		GetRPCManager().SendRPC("RPC_PermitManager", "VerifyButtonsPermission", new Param1<ref array<string>>(perms) ,true);
	}
	
	/*
		This gets called even when you ShowMenu() init only happens once
	*/
   	override Widget Init()
   	{
		if (!m_Init)
		{
			layoutRoot   	    = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/AdminToolBar/VPPAdminHud.layout" );
	  	  	m_WrapSpacerWidget  = WrapSpacerWidget.Cast( layoutRoot.FindAnyWidget( "WrapSpacerWidget") );
			m_Init = true;
			return layoutRoot;
		}
		return layoutRoot;
   	}
	
	/* 
		@DOC: external modules shall override this function to insert their custom buttons. Buttons' name will be the permission sent to server to check
		      meaning you should use the same name on server when you add your custom permission to the table using AddPermissionType();
	*/
	void DefineButtons()
	{
		
	}
	
	private void InsertButton(string classname, string image, string desc)
	{
		m_DefinedButtons.Insert(new Param3<string,string,string>(classname,image,desc));
	}
	
	/* Called after server replies back to VerifyButtonsPermission*/
	private void InitButtons()
	{
		foreach(Param3<string,string,string> data : m_DefinedButtons)
		{
			AddButton(data.param1,data.param2,data.param3);
		}
	}
	
	void VerifyButtonsPermission(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref map<string,bool>> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Client)
        {
			m_ButtonPerms = data.param1;
			m_ButtonPerms.Copy(data.param1);
			InitButtons();
		}
	}
	
	override void Update(float timeslice)
	{
		super.Update(timeslice);

		foreach(ref AdminHudSubMenu m : M_SUB_MENUS)
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
	
	/* Function handel base menu buttons (weather,settings,player,object,item) */
	void HandelClick(Widget btnW, string btnName)
	{
		foreach(VPPButton button : m_Buttons){
			if (button.ButtonName == btnName && button.Button == ButtonWidget.Cast(btnW))
			{
				autoptr AdminHudSubMenu menu = GetSubMenuByType(button.ButtonName.ToType());
				if (menu)
				{
					if (menu.IsSubMenuVisible())
						menu.HideSubMenu();
					else
						menu.ShowSubMenu();
					
					return;
				}
				CreateSubMenu(button.ButtonName.ToType());
			}
		}
	}
	
	void CreateSubMenu(typename subMenuType)
	{
		if (GetSubMenuByType(subMenuType) == NULL)
		{
			autoptr AdminHudSubMenu menu = AdminHudSubMenu.Cast(subMenuType.Spawn());
			menu.OnCreate(layoutRoot);
			menu.ShowSubMenu();
			M_SUB_MENUS.Insert(menu);
		}
	}
	
	AdminHudSubMenu GetSubMenuByType(typename subMenuType)
	{
		for(int i = 0; i < M_SUB_MENUS.Count(); i++)
		{
			autoptr AdminHudSubMenu SubMenu = M_SUB_MENUS[i];
			
			if (SubMenu.ClassName().ToType() == subMenuType)
			{
				return SubMenu;
			}
		}
		return NULL;
	}
	
	/* Verifies players button permissions then adds button to hud.*/
	private void AddButton(string buttonName, string icon, string desc)
	{
		if (!CheckDuplicateButton(buttonName,true))
		{
			bool state;
			if (m_ButtonPerms.Find(buttonName,state))
			{
				if (state)
				{
					Widget buttonWidget = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/AdminToolBar/VPPButton.layout", m_WrapSpacerWidget );
					m_Buttons.Insert(new VPPButton(buttonWidget,buttonName,icon,desc));
				}else{
					//Disable button if already exists and perm changed to false
					autoptr VPPButton toDelete = GetButtonByName(buttonName);
					if (toDelete != null)
					{
						Print("[VPPAdminHud] AddButton():: Removing Active Button "+toDelete.ButtonName + " permission to use was revoked!");
						m_Buttons.RemoveItem(toDelete);
						delete toDelete;
					}
					Print("[VPPAdminHud] AddButton():: Unable to Init Button: "+buttonName + " insufficient permission!");
				}
			}
		}else{
			Print("[VPPAdminHud] AddButton():: Attempted to add multiple of same button!");
		}
	}
	
	private bool CheckDuplicateButton(string buttonName, bool deleteDuplicate = false)
	{
		if (m_Buttons.Count() == 0) return false;
		
		foreach(ref VPPButton btn : m_Buttons)
		{
			if (btn != null && buttonName == btn.ButtonName)
			{
				if (deleteDuplicate)
				{
					m_Buttons.RemoveItem(btn);
					delete btn;
					return false;
				}
			}
		}
		return false;
	}
	
	private VPPButton GetButtonByName(string name)
	{
		foreach(ref VPPButton btn : m_Buttons)
		{
			if (btn.ButtonName == name)
			{
				return btn;
			}
		}
		return null;
	}
	
	override bool OnClick( Widget w, int x, int y, int button )
	{
		foreach(ref VPPButton btn: m_Buttons)
		{
			if (w == btn.Button)
			{
				int state = GetGame().GameScript.CallFunctionParams(this, "HandelClick", NULL, new Param2<Widget,string>( w, btn.ButtonName));
				
				if (state == -1)
					Print("[VPPAdminHud] Call Back Failed for registered button: "+btn.ButtonName);
			}
		}
		return super.OnClick( w, x, y, button );
	}
};