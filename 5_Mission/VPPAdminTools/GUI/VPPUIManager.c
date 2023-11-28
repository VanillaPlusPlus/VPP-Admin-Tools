class VPPUIManager extends PluginBase
{
	static ref array<Widget> WIDGET_PTRs = {}; //Only for draw-to-select feature, hold a weak ptr to widgets to search for.

	private ref array<ref VPPScriptedMenu> M_SCRIPTED_UI_INSTANCES;
	private bool m_LockCommandKeybinds;
	private bool m_IsDraggingSubWindow;
	private bool m_IsSelectBoxDrawing;
	Widget m_DrawCanvas;
	CanvasWidget m_DrawCanvasWidget;

	void VPPUIManager()
	{
		M_SCRIPTED_UI_INSTANCES = new array<ref VPPScriptedMenu>;
	}
	
	override void OnInit()
	{
		Print("Client: VPPUIManager Plugin Init!");
		m_DrawCanvas = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.PlayerESPCanvas);
		m_DrawCanvasWidget = CanvasWidget.Cast(m_DrawCanvas.FindAnyWidget("CanvasWidget"));
	}
	
	void ~VPPUIManager()
	{
		if (!GetGame().IsClient())
			return;
		
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

		if (m_DrawCanvas)
			m_DrawCanvas.Unlink();
	}
	
	/*
		IMPORTANT NOTE: Don't forget to set a const int to use for initing your menu! We are still using vanilla method to insert menu into pool! (due to ease of life with all the click events)
		Also, make sure to check out class MissionBase! (You need to add your menu to the switch case) -DaOne
		menuID is for now only used by game base script class 'UIManager' we shall use typenames to get/set menus
	*/
	bool CreateMenu(typename menuType, int menuID, UIScriptedMenu menuParent = NULL) //passing a parent is an option if you wish to overlap another menu with yours
	{
		if (GetMenuByType(menuType) == NULL)
		{
			VPPScriptedMenu VPPMenu;
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
	
	VPPScriptedMenu GetMenuByType(typename menuType)
	{
		foreach(VPPScriptedMenu menu : M_SCRIPTED_UI_INSTANCES){
			if (menu && menu.GetType() == menuType){
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
			VPPScriptedMenu menu = M_SCRIPTED_UI_INSTANCES[i];
			if (menu.GetType() == menuType)
			{
	    		M_SCRIPTED_UI_INSTANCES.Remove(i);
				Print("[VPPUIManager]: Destroyed Scripted UI: "+ menu.GetType());
	    	}
		}
	}
	
	void HookConfirmationDialog(Widget target, Widget parent, Class callBack, string funcName, int diagType, string title, string msg, bool allowchars = false)
	{
		ConfirmationEventHandler confirmClass;
		target.GetScript(confirmClass);
		if (confirmClass) confirmClass.InitEvent(callBack,funcName,diagType,title,msg,parent,allowchars);
	}

	VPPDialogBox CreateDialogBox(Widget parent, bool forceNull = false)
	{
		Widget dialog;
		VPPDialogBox handler;
		if (parent == null && forceNull)
		{
			dialog = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPDialogBox, null);
			dialog.GetScript(handler);
			handler.MakeSmall();
			return handler;
		}
		
		if (parent != null)
		{
			if (parent.FindAnyWidget("Main") == null)
				dialog = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPDialogBox, parent.FindAnyWidget("PanelConfirmationBox"));
			else
				dialog = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPDialogBox, parent.FindAnyWidget("Main"));
			
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

	void SetDraggingWindow(bool state)
	{
		m_IsDraggingSubWindow = state;
	}

	bool IsDraggingWindow()
	{
		return m_IsDraggingSubWindow;
	}
	
	bool IsTyping()
	{
		if (GetGame().GetUIManager().GetMenu() == null) return false;
		Widget underCursor = GetWidgetUnderCursor();
		UIScriptedMenu chatInput = GetGame().GetUIManager().FindMenu(MENU_CHAT_INPUT);
				
		if (underCursor && GetGame().GetUIManager().IsCursorVisible() && (underCursor.GetTypeID() == EditBoxWidgetTypeID || underCursor.GetTypeID() == MultilineEditBoxWidgetTypeID))
			return true;
		
		if (chatInput != null && chatInput.IsVisible())
			return true;
		
		return false;
	}

	void DrawSelectionBoxEx()
	{
		int x, y;
        GetMousePos(x, y);
        thread DrawSelectionBox(x, y);
	}

	private void DrawSelectionBox(int mouse_x, int mouse_y)
    {
        int current_x, current_y;
        while ((GetGame().GetMouseState(MouseState.RIGHT) & MB_PRESSED_MASK) && !IsDraggingWindow() && !g_Game.IsLeftCtrlDown())
        {
            m_IsSelectBoxDrawing = true;
            GetMousePos(current_x, current_y);
            m_DrawCanvasWidget.Clear();

            if (Math.AbsInt(mouse_x - current_x) > 15 || Math.AbsInt(mouse_y - current_y) > 15) 
            {
                m_DrawCanvasWidget.DrawLine(mouse_x, mouse_y, current_x, mouse_y, 1, ARGB(255,0,0,255));
                m_DrawCanvasWidget.DrawLine(mouse_x, mouse_y, mouse_x, current_y, 1, ARGB(255,0,0,255));
                m_DrawCanvasWidget.DrawLine(mouse_x, current_y, current_x, current_y, 1, ARGB(255,0,0,255));
                m_DrawCanvasWidget.DrawLine(current_x, mouse_y, current_x, current_y, 1, ARGB(255,0,0,255));
                
                int x_avg = (mouse_x + current_x) / 2;
                m_DrawCanvasWidget.DrawLine(x_avg, mouse_y, x_avg, current_y, current_x - mouse_x, ARGB(50,0,0,255)); 
                
                int total = VPPUIManager.WIDGET_PTRs.Count();
                if (total > 0)
                {
                	for (int i = 0; i < total; ++i)
                	{
                		Widget w = VPPUIManager.WIDGET_PTRs[i];
                		if (!w)
                			continue;

                		Class cbPtr;
		            	w.GetUserData(cbPtr);
		            	if (!cbPtr)
		            		continue;

                		float posX, posY;
                        w.GetPos(posX, posY);

                        //Check is within drawbox
                        bool isInBounds = ((posX < Math.Max(mouse_x, current_x) && posX > Math.Min(mouse_x, current_x)) && (posY < Math.Max(mouse_y, current_y) && posY > Math.Min(mouse_y, current_y)));
                        //Preform callback
                        if (isInBounds)
                        {
                        	GetGame().GameScript.CallFunction(cbPtr, "OnWidgetDragSelect", NULL, true);
                        }
                        else if (!g_Game.IsLShiftHolding())
                        {
                        	GetGame().GameScript.CallFunction(cbPtr, "OnWidgetDragSelect", NULL, false);
                        }
                	}
                }
            }
            Sleep(10);
        }
        m_IsSelectBoxDrawing = false;
        m_DrawCanvasWidget.Clear();
    }

    bool IsSelectionBoxDrawing()
    {
    	return m_IsSelectBoxDrawing;
    }
};

VPPUIManager GetVPPUIManager()
{
	if ( GetPluginManager() && GetPluginManager().GetPluginByType(VPPUIManager))
		return VPPUIManager.Cast(GetPluginManager().GetPluginByType(VPPUIManager));

	return NULL;
};

/*
Create a new menu example: VPPAdminHud adminUI = VPPAdminHud.Cast(GetVPPUIManager().CreateMenu(VPPAdminHud,VPP_ADMIN_HUD));
*/
//Menu Instance base type (use instead of UIScriptedMenu)
class VPPScriptedMenu extends UIScriptedMenu
{
	private bool m_AllowSelectBoxDraw;

	bool menuStatus; //True == showing
	bool m_GameFocus;
	bool m_toggleFocus;

	void ~VPPScriptedMenu()
	{
		if (!GetGame().IsClient())
			return;
		
		UnlockPlayerControl();
		
		if ( GetVPPUIManager() != NULL)
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
		}
		else if (input.LocalRelease("UAFocusOnGame", false) && !m_toggleFocus && menuStatus)
		{
			LockPlayerControl();
		}
	}

	//Toggle of control focus
	void HandleGameFocus()
	{
		m_toggleFocus = !m_toggleFocus;

		if (m_toggleFocus)
		{
			SetFocus(null);
			UnlockPlayerControl();
		}else{
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
			VPPScriptedMenu menu = VPPScriptedMenu.Cast(GetGame().GetUIManager().ShowScriptedMenu(this, NULL));
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
		//GetGame().GetMission().AddActiveInputExcludes({"VPPCamControls"});
		GetGame().GetUIManager().ShowUICursor( true );
		GetGame().GetMission().GetHud().Show( false );
		m_GameFocus = false;
	}
	
	void UnlockPlayerControl()
	{
		if (IsFreeCamActive())
		{
			GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
			//GetGame().GetMission().RemoveActiveInputExcludes({"VPPCamControls"});
		}
		GetVPPUIManager().SetKeybindsStatus(false); //unlock shortcut keys
		GetGame().GetMission().PlayerControlEnable(true);
		GetGame().GetInput().ResetGameFocus();
		GetGame().GetUIManager().ShowUICursor( false );
		GetGame().GetMission().GetHud().Show( true );
		m_GameFocus = true;
	}
	
	void AllowSelectBoxDraw(bool allow)
    {
    	m_AllowSelectBoxDraw = allow;
    }

    bool CanDrawSelectBox()
    {
    	return m_AllowSelectBoxDraw && !GetVPPUIManager().IsSelectionBoxDrawing();
    }

	/* Widget Events */
	override bool OnClick( Widget w, int x, int y, int button )
	{
		return super.OnClick( w, x, y, button );
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button == MouseState.RIGHT)
        {
			if (!IsShowing() || !CanDrawSelectBox())
				return false;

            if (!GetVPPUIManager().IsDraggingWindow() && !GetVPPUIManager().IsTyping())
            {
                GetVPPUIManager().DrawSelectionBoxEx();
                return true;
            }
        }
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