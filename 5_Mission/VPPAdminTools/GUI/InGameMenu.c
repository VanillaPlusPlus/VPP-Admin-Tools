/*modded class InGameMenu
{
    private Widget m_CustomButton;

    override Widget Init()
    {
        super.Init();
		
		m_CustomButton	 = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/InGameMenuButton.layout", layoutRoot.FindAnyWidget( "top" ));
        return layoutRoot;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        super.OnClick(w, x, y, button);
        
        if (w == m_CustomButton)
        {
            Print("Clicked On m_CustomButton");
            return true;
        }
        return super.OnClick(w, x, y, button);
    }
};*/