/*modded class VPPAdminHud
{
	override void DefineButtons()
	{
		super.DefineButtons();
		InsertButton("CustomSubMenu" , "set:dayz_gui_vpp image:vpp_icon_obj_editor","This is an example description");
	}
};

class CustomSubMenu extends AdminHudSubMenu
{
	private CanvasWidget canvas;

	void CustomSubMenu()
	{
		Print("CustomSubMenu()");
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		
		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/CustomSubMenu.layout");
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		canvas = CanvasWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "CanvasWidget") );
		ShowSubMenu();
	}
	
	void DrawUpdate(float tDelta)
	{
		if (GetGame().GetMouseState(MouseState.LEFT) & 0x80000000)
		{
			float wX,wY;
			float width, height;
			canvas.GetScreenSize(wX, wY);
			canvas.GetSize(width, height);
			int oX, oY;
			int sX, sY;
			GetGame().GetMousePos(oX, oY);
			GetGame().GetScreenSize(sX, sY);
            float size = 3.5;

			float x = (width + sX)  - (wX + oX);
            float y = (height + sY) - (wY - oY);
			
			Print("x: " + x + " y: " + y);
			
            canvas.DrawLine(x, y, x + 6.5, y, size, ARGB(255,255,255,255) );
		}
	}

	void OnUpdate(float timeslice)
	{
		
	}
};
*/
/*
/* FUNCTIONS SUMMARY */

/* 
   void InsertButton(string buttonClassName, string icon) 
   @Param0 given name for the button, important to distinguish from other buttons, also used to determine which button was clicked incase call back method is shared NOTE: This string MUST be unique from other buttons
   @Param1 icon for your button either direct path or template from .imageset eg: set:dayz_gui_vpp image:vpp_icon_obj_editor image size should be 135px by 135px

   Note: Upon button click, call back will pass two params to your specified method( Widget btnW, string btnName) @Param0: Clicked button  @Param1: button name
*/