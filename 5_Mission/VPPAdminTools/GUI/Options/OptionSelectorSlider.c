modded class OptionSelectorSlider
{
	protected bool m_HasChanged;

	override bool OnChange( Widget w, int x, int y, bool finished )
	{
		m_HasChanged = true;
		return super.OnChange(w, x, y, finished);
	}

	bool HasChanged()
	{
		return m_HasChanged;
	}

	void SetHasChanged(bool state)
	{
		m_HasChanged = state;
	}
};