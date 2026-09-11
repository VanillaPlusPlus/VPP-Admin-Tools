/*
	One player row in the Spectate menu list.
	Row body click = select (fills the target card); eye button = select + spectate now.
	The admin's own row shows a green name and no spectate button.
*/
class SpectatePlayerEntry : VPPPlayerTemplate
{
	protected MenuSpectateTools m_Owner;
	protected string            m_UserId;
	protected string            m_UserName;
	protected bool              m_IsSelf;

	protected TextWidget        m_TxtRowName;
	protected ButtonWidget      m_BtnRowSpectate;

	void SpectatePlayerEntry(GridSpacerWidget grid, MenuSpectateTools owner, string userName, string userId, bool isSelf)
	{
		m_Owner    = owner;
		m_UserName = userName;
		m_UserId   = userId;
		m_IsSelf   = isSelf;

		m_LayoutPath = VPPATUIConstants.SpectatePlayerEntry;
		m_EntryBox   = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);

		m_TxtRowName     = TextWidget.Cast(m_EntryBox.FindAnyWidget("TxtRowName"));
		m_BtnRowSpectate = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("BtnRowSpectate"));

		m_TxtRowName.SetText(userName);

		if (m_IsSelf)
		{
			m_TxtRowName.SetColor(ARGB(255, 76, 175, 80)); //green = you
			m_BtnRowSpectate.Show(false);
		}

		m_EntryBox.SetHandler(this);
	}

	void ~SpectatePlayerEntry()
	{
		if (m_EntryBox != null)
			m_EntryBox.Unlink();
	}

	string GetUserId()
	{
		return m_UserId;
	}

	string GetUserName()
	{
		return m_UserName;
	}

	bool IsSelf()
	{
		return m_IsSelf;
	}

	void SetSelected(bool state)
	{
		if (state)
			m_EntryBox.SetColor(ARGB(200, 45, 90, 160));  //selection blue
		else
			m_EntryBox.SetColor(ARGB(217, 27, 30, 34));   //bg-row
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Owner == null)
			return false;

		if (w == m_BtnRowSpectate)
		{
			m_Owner.OnRowSpectate(m_UserId);
			return true;
		}

		return false;
	}

	//row-body select: panels don't emit OnClick on PC — use mouse-down like BuildingEntry
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (m_Owner == null)
			return false;

		if (button == MouseState.LEFT && w == m_EntryBox)
		{
			m_Owner.OnRowSelected(m_UserId);
			return true;
		}

		return false;
	}
};
