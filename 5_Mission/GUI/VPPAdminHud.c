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
		toolTip.SetTitle("Information:");
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
		InsertButton("MenuPlayerManager" , "set:dayz_gui_vpp image:vpp_icon_players", "Manage players, teleport,kill,send message,heal,view information about players (guid,name,status) & more");
		InsertButton("MenuItemManager" , "set:dayz_gui_vpp image:vpp_icon_item_manager", "Spawn items on players, on ground, at crosshairs, create, edit and delete item sets & more!");
		InsertButton("MenuTeleportManager" , "set:dayz_gui_vpp image:vpp_icon_teleport_manager", "Manage teleport presets, add new and edit. Mass teleport players & map teleport.");
		InsertButton("EspToolsMenu" , "set:dayz_gui image:icon_refresh", "Esp....");
		InsertButton("MenuServerManager" , "set:dayz_gui_vpp image:vpp_icon_settings", "Control Server settings, view player activity, restart server, monitor performance & more!");
		InsertButton("MenuWeatherManager" , "set:dayz_gui_vpp image:vpp_icon_cloud", "Manage server weather settings, load, create delete presets, set time and change forecast.");
		InsertButton("MenuObjectManager" , "set:dayz_gui_vpp image:vpp_icon_obj_editor", "Place custom mission objects, create, load, edit presets & more!");
		InsertButton("MenuBansManager" , "set:dayz_gui_vpp image:vpp_icon_bans_editor", "Manage V++ Server ban list, add, remove, updated issued bans.");
		InsertButton("MenuPermissionsEditor" , "set:dayz_gui_vpp image:vpp_icon_perms_editor", "Manage toolset permissions & roles, create new user groups, edit and delete.");
		//InsertButton("MenuLogsViewer" , "set:dayz_gui_vpp image:vpp_icon_settings", "View V++ custom server logs.");
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
	
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return super.OnMouseButtonDown(w,x,y,button);
	}
};