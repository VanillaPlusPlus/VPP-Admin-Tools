class EditBoxEventHandler extends ScriptedWidgetEventHandler
{
	protected const ref array<string> m_illegalchars = {"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","s","t","u","v","w","x","y","z","!","@","#","$","%","^","&","*","(",")","_",","/*,"."*/,"-","/","\\","+","=","~","`"};
	protected EditBoxWidget    m_root;
	protected int              m_step = 1; //How much to -- or ++ when scrolling ( 0 will be ++/-- 1 number at a time )
	protected bool 			   m_AllowIllegalChar;

	void OnWidgetScriptInit(Widget w)
	{
		m_root = EditBoxWidget.Cast(w);
		m_root.SetHandler(this);
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		string text = m_root.GetText();
		if (ContainsIllegalChar(text)){
			m_root.SetText("0");
			m_root.Update();
			return true;
		}
		return false;
	}
	
	private bool ContainsIllegalChar(string text)
	{
		if (m_AllowIllegalChar) return false;
		
		foreach(string char : m_illegalchars){
			int fuckYouDayZ = text.ToLower();
			if (text.Contains(char)){
				return true;
			}
		}
		return false;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		string oldValue = m_root.GetText();
		if (wheel <= -1){
			m_root.SetText((oldValue.ToInt() - m_step).ToString());
		}else{
			m_root.SetText((oldValue.ToInt() + m_step).ToString());
		}
		m_root.Update();
		return true;
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w == m_root)
		{
			SetFocus(null);
			return true;
		}
		return false;
	}
	
	void SetStep(int step)
	{
		m_step = step;
	}
	
	void AllowCharInput()
	{
		m_AllowIllegalChar = true;
	}
};