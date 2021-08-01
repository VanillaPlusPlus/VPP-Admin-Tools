modded class MissionBase
{
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		UIScriptedMenu menu = NULL;
        menu = super.CreateScriptedMenu(id);
        if (!menu)
        {
            switch (id)
            {
				case VPP_ADMIN_HUD:
                menu = new VPPAdminHud;
                break;

                case DRAG_TEST:
                menu = new DragSelect;
                break;
            }
            if (menu)
            {
                menu.SetID(id);
            }
        }
        return menu;
	}
};