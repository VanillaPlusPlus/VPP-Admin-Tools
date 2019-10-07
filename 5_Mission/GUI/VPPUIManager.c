class VPPUIManager extends PluginBase
{
	private ref array<ref VPPScriptedMenu> M_SCRIPTED_UI_INSTANCES;
	private bool m_LockCommandKeybinds;
	
	void VPPUIManager()
	{
		M_SCRIPTED_UI_INSTANCES = new array<ref VPPScriptedMenu>;
	}
	
	override void OnInit()
	{
		Print("Client: VPPUIManager Plugin Init!");
	}
	
	void ~VPPUIManager()
	{
		if (M_SCRIPTED_UI_INSTANCES)
		{
			foreach(VPPScriptedMenu menu : M_SCRIPTED_UI_INSTANCES)
			{
				if (menu != null)
				{
					Print("~VPPUIManager() :: Delete: "+menu);
					delete menu;
				}
			}
		}
	}
	
	/*
		IMPORTANT NOTE: Don't forget to set a const int to use for initing your menu! We are still using vanilla method to insert menu into pool! (due to ease of life with all the click events)
		Also, make sure to check out class MissionBase! (You need to add your menu to the switch case) -DaOne
		menuID is for now only used by game base script class 'UIManager' we shall use typenames to get/set menus
	*/
	bool CreateMenu(typename menuType, int menuID, ref UIScriptedMenu menuParent = NULL) //passing a parent is an option if you wish to overlap another menu with yours
	{
		if (GetMenuByType(menuType) == NULL)
		{
			ref VPPScriptedMenu VPPMenu;
			if (Class.CastTo(VPPMenu,GetGame().GetUIManager().EnterScriptedMenu(menuID, menuParent))){
				M_SCRIPTED_UI_INSTANCES.Insert( VPPMenu );
				Print("[VPPUIManager] Menu "+VPPMenu.GetType() + " was created! with ID: "+ menuID);
				VPPMenu.OnInit();
				return true;
			}
		}
		
		Print("[VPPUIManager] Unable to create menu: "+menuType.ToString() + " with ID: "+menuID + " Instance is already existing! Use ShowMenu() ?");
		return false;
	}
	
	ref VPPScriptedMenu GetMenuByType(typename menuType)
	{
		foreach(ref VPPScriptedMenu menu : M_SCRIPTED_UI_INSTANCES){
			if (menu.GetType() == menuType){
	    			return menu;
	    		}
		}
		return NULL;
	}
	
	/*
	Note: this only removes instance from M_SCRIPTED_UI_INSTANCES array, called within the destrucotr of VPPScriptedMenu instances
	*/
	void DestroyMenuInstanceByType(typename menuType)
	{
		for(int i = 0; i < M_SCRIPTED_UI_INSTANCES.Count(); i++)
		{
			ref VPPScriptedMenu menu = M_SCRIPTED_UI_INSTANCES[i];
			if (menu.GetType() == menuType)
			{
	    		M_SCRIPTED_UI_INSTANCES.Remove(i);
				Print("[VPPUIManager]: Destroyed Scripted UI: "+ menu.GetType());
	    	}
		}
	}
	
	void HookConfirmationDialog(Widget target, ref Widget parent, Class callBack, string funcName, int diagType, string title, string msg, bool allowchars = false)
	{
		ref ConfirmationEventHandler confirmClass;
		target.GetScript(confirmClass);
		if (confirmClass) confirmClass.InitEvent(callBack,funcName,diagType,title,msg,parent,allowchars);
	}

	ref VPPDialogBox CreateDialogBox(Widget parent, bool forceNull = false)
	{
		ref Widget dialog;
		autoptr VPPDialogBox handler;
		if (parent == null && forceNull)
		{
			dialog = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/UIHelpers/VPPDialogBox.layout", null);
			dialog.GetScript(handler);
			handler.MakeSmall();
			return handler;
		}
		
		if (parent != null)
		{
			if (parent.FindAnyWidget("Main") == null)
				dialog = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/UIHelpers/VPPDialogBox.layout", parent.FindAnyWidget("PanelConfirmationBox"));
			else
				dialog = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/UIHelpers/VPPDialogBox.layout", parent.FindAnyWidget("Main"));
			
			dialog.GetScript(handler);
			if (handler != null) return handler;
		}else{
			Print("[VPPUIManager] CreateDialogBox() Error unable to create! parent widget is NULL");
		}
		return null;
	}
	
	void DisplayError(string message, float lifeTime = 5.0, string icon = "set:ccgui_enforce image:Icon40Emergency")
	{
		NotificationSystem.AddNotificationExtended(lifeTime, "Error:", message, icon);
	}
	
	void DisplayNotification(string message, string title = "Admin Notification:", float lifeTime = 3.0,string icon = "set:ccgui_enforce image:MapUserMarker")
	{
		NotificationSystem.AddNotificationExtended(lifeTime, title, message, icon);
	}
	
	//true == locked
	void SetKeybindsStatus(bool state)
	{
		m_LockCommandKeybinds = state;
	}
	
	bool GetKeybindsStatus()
	{
		return m_LockCommandKeybinds;
	}
	
	bool IsTyping()
	{
		if (GetGame().GetUIManager().GetMenu() == null) return false;
		autoptr Widget underCursor = GetWidgetUnderCursor();
		autoptr UIScriptedMenu chatInput = GetGame().GetUIManager().FindMenu(MENU_CHAT_INPUT);
				
		if (underCursor != null && (underCursor.GetTypeID() == EditBoxWidgetTypeID || underCursor.GetTypeID() == MultilineEditBoxWidgetTypeID))
			return true;
		
		if (chatInput != null && chatInput.IsVisible())
			return true;
		
		return false;
	}
};


ref VPPUIManager GetVPPUIManager()
{
	return VPPUIManager.Cast(GetPluginManager().GetPluginByType(VPPUIManager));
}

/*
Create a new menu example: VPPAdminHud adminUI = VPPAdminHud.Cast(GetVPPUIManager().CreateMenu(VPPAdminHud,VPP_ADMIN_HUD));
*/
//Menu Instance base type (use instead of UIScriptedMenu)
class VPPScriptedMenu extends UIScriptedMenu
{
	bool menuStatus; //True == showing
	bool m_GameFocus;
	bool m_toggleFocus;

	void ~VPPScriptedMenu()
	{
		GetVPPUIManager().DestroyMenuInstanceByType(GetType());
		Print("Destroy Menu: "+GetType());
	}
	
	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (!menuStatus) return;
		
		Input input = GetGame().GetInput();

		if (input.LocalDbl("UAFocusOnGame", false))
		{
			if (m_toggleFocus)
			{
				m_toggleFocus = false;
			}
			else
			{
				m_toggleFocus = true;
				SetFocus(null);
				UnlockPlayerControl();
			}
		}

		if (input.LocalHold("UAFocusOnGame", false))
		{
			if (menuStatus && !m_GameFocus)
			{
				SetFocus(null);
				UnlockPlayerControl();
			}
		}else if (input.LocalRelease("UAFocusOnGame", false) && !m_toggleFocus && menuStatus){
			LockPlayerControl();
		}
	}

	/*
		Called When menu is created at VPPUIManager::CreateMenu();
	*/
	void OnInit()
	{
		//lock Control and show mouse cursor + set menu state to showing
		if (GetGame().GetUIManager().GetMenu().ClassName().ToType() == GetType() && !IsShowing())
		{
			menuStatus = true;
			LockPlayerControl();
			Print("[VPPScriptedMenu]: Initializing Scripted UI: "+ GetType());
		}else{
			Print("[VPPScriptedMenu]: FAILED to Initialize Scripted UI: "+ GetType());
		}
	}
	
	bool IsShowing()
	{
	   return menuStatus;
	}
	
	void DestroyMenu()
	{
	   delete this;
	}
	
	void ShowMenu()
	{
		if (!IsShowing() && GetGame().GetUIManager().GetMenu() == NULL){
			ref VPPScriptedMenu menu = VPPScriptedMenu.Cast(GetGame().GetUIManager().ShowScriptedMenu(this, NULL));
			if (menu){
				LockPlayerControl();
				menuStatus = true;
			}else{
				Print("[VPPScriptedMenu] Unable to show menu: "+ GetType() + " Something went wrong! `ShowScriptedMenu` returned NULL!");
			}
		}else{
			Print("[VPPScriptedMenu] Unable to show menu: "+ GetType() + " Another Menu is on screen OR menu is already showing!");
		}
	}
	

	void HideMenu()
	{
		if (GetGame().GetUIManager().GetMenu() == null) return;
		
		if (GetGame().GetUIManager().GetMenu().ClassName().ToType() == GetType())
		{
			GetGame().GetUIManager().HideScriptedMenu(this);
			UnlockPlayerControl();
			menuStatus = false;
		}else{
			Print("[VPPScriptedMenu] Could not close menu: "+ GetType() + " Menu was not found!");
		}
	}
	
	void LockPlayerControl()
	{
		GetVPPUIManager().SetKeybindsStatus(true); //Lock shortcut keys
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		GetUApi().ActivateExclude("VPPCamControls");
		GetUApi().UpdateControls();
		GetGame().GetUIManager().ShowUICursor( true );
		GetGame().GetMission().GetHud().Show( false );
		m_GameFocus = false;
	}
	
	void UnlockPlayerControl()
	{
		if (IsFreeCamActive())
		{
			GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		}
		GetVPPUIManager().SetKeybindsStatus(false); //unlock shortcut keys
		GetGame().GetMission().PlayerControlEnable();
		GetGame().GetInput().ResetGameFocus();
		GetGame().GetUIManager().ShowUICursor( false );
		GetGame().GetMission().GetHud().Show( true );
		m_GameFocus = true;
	}
	
	/* Widget Events */
	override bool OnClick( Widget w, int x, int y, int button )
	{
		return super.OnClick( w, x, y, button );
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return super.OnMouseButtonDown( w, x, y, button);
	}

	override bool OnMouseWheel(Widget  w, int  x, int  y, int wheel)
	{
		return super.OnMouseWheel( w, x, y, wheel);
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		return super.OnChange( w, x, y, finished);
	}
	
     override bool OnMouseEnter( Widget w, int x, int y )
	{
		return super.OnMouseEnter( w, x, y );
	}
	
	override bool OnMouseLeave( Widget w, Widget enterW, int x, int y )
	{
		return super.OnMouseLeave( w, enterW, x, y );
	}
	
	string GetTypeName(){
		return ClassName();
	}

	typename GetType(){
		return GetTypeName().ToType();
	}
};