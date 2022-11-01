#ifdef COMPILE_EXAMPLES
modded class VPPAdminHud
{
	override void DefineButtons()
	{
		super.DefineButtons();
		InsertButton("CustomSubMenu" , "Custom Menu Name", "set:dayz_gui_vpp image:vpp_icon_obj_editor", "This is an example description");
	}
};

class CustomSubMenu extends AdminHudSubMenu
{
	private bool m_Init;
	
	void CustomSubMenu()
	{
		//GetRPCManager().AddRPC("RPC_MenuPlayerManager", "HandlePlayerStats", this, SingleplayerExecutionType.Client);
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		
		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuPlayerManager);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_Init = true;
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		
		if (!IsSubMenuVisible() && M_SUB_WIDGET == null)
			return;
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		return super.OnClick(w, x, y, button);
	}

	override void HideSubMenu()
	{
		super.HideSubMenu();
	}
	
	override void ShowSubMenu()
	{
		super.ShowSubMenu();
	}
};
#endif