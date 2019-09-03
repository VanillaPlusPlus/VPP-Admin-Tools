class BanDurationInput extends ScriptedWidgetEventHandler
{
	protected Widget  	   m_root;
	protected ImageWidget  m_ImgInfo;
	protected ButtonWidget m_Apply;
	protected ButtonWidget m_Close;
	protected CheckBoxWidget m_PermaTick; 
	
	protected EditBoxWidget m_InputYear;
	protected EditBoxWidget m_InputMonth;
	protected EditBoxWidget m_InputDay;
	protected EditBoxWidget m_InputHour;
	protected EditBoxWidget m_InputMinute;
	
	protected ButtonWidget m_brtnYrDec;
	protected ButtonWidget m_brtnMnthDec;
	protected ButtonWidget m_brtnDayDec;
	protected ButtonWidget m_brtnHrDec;
	protected ButtonWidget m_brtnMinDec;
	
	protected ButtonWidget m_brtnYrInc;
	protected ButtonWidget m_brtnMnthInc;
	protected ButtonWidget m_brtnDayInc;
	protected ButtonWidget m_brtnHrInc;
	protected ButtonWidget m_brtnMinInc;
	
	private ref MenuBansManager m_RootClass;
	
	void OnWidgetScriptInit(Widget w)
	{
		m_root = w;
		m_root.SetHandler(this);
		
		if (m_root != null)
		{
			m_Apply = ButtonWidget.Cast(m_root.FindAnyWidget("btnApply"));
			m_Close = ButtonWidget.Cast(m_root.FindAnyWidget("btnCancel"));
			m_ImgInfo = ImageWidget.Cast(m_root.FindAnyWidget("ImgInfo"));
			m_PermaTick = CheckBoxWidget.Cast(m_root.FindAnyWidget("PermaTick"));
			autoptr ToolTipHandler toolTip;
			m_ImgInfo.GetScript(toolTip);
			toolTip.SetTitle("Information:");
			toolTip.SetContentText("Set the unban date of when the selected ban gets revoked/removed.The date is based-off the time set on the machine this DayZ server is running on.");
			
			
			m_InputYear	    = EditBoxWidget.Cast(m_root.FindAnyWidget("InputYear"));
			m_InputMonth	= EditBoxWidget.Cast(m_root.FindAnyWidget("InputMonth"));
			m_InputDay		= EditBoxWidget.Cast(m_root.FindAnyWidget("InputDay"));
			m_InputHour		= EditBoxWidget.Cast(m_root.FindAnyWidget("InputHour"));
			m_InputMinute	= EditBoxWidget.Cast(m_root.FindAnyWidget("InputMinute"));
			
			m_brtnYrDec		= ButtonWidget.Cast(m_root.FindAnyWidget("brtnYrDec"));
			m_brtnMnthDec	= ButtonWidget.Cast(m_root.FindAnyWidget("brtnMnthDec"));
			m_brtnDayDec	= ButtonWidget.Cast(m_root.FindAnyWidget("brtnDayDec"));
			m_brtnHrDec		= ButtonWidget.Cast(m_root.FindAnyWidget("brtnHrDec"));
			m_brtnMinDec	= ButtonWidget.Cast(m_root.FindAnyWidget("brtnMinDec"));
			
			m_brtnYrInc		= ButtonWidget.Cast(m_root.FindAnyWidget("brtnYrInc"));
			m_brtnMnthInc	= ButtonWidget.Cast(m_root.FindAnyWidget("brtnMnthInc"));
			m_brtnDayInc	= ButtonWidget.Cast(m_root.FindAnyWidget("brtnDayInc"));
			m_brtnHrInc		= ButtonWidget.Cast(m_root.FindAnyWidget("brtnHrInc"));
			m_brtnMinInc	= ButtonWidget.Cast(m_root.FindAnyWidget("brtnMinInc"));
			
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdateInputs, 10, true);
		}
	}
	
	void ~BanDurationInput()
	{
		if (m_root != null)
			m_root.Unlink();
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.UpdateInputs);
	}
	
	void UpdateInputs()
	{
		int input;
		input = m_InputMonth.GetText().ToInt();
		if (input > 12 || input < 1)
			m_InputMonth.SetText("1");
		
		input = m_InputDay.GetText().ToInt();
		if (input > 31 || input < 1)
			m_InputDay.SetText("1");
		
		input = m_InputHour.GetText().ToInt();
		if (input > 24 || input < 0)
			m_InputHour.SetText("1");
		
		input = m_InputMinute.GetText().ToInt();
		if (input > 59 || input < 0)
			m_InputMinute.SetText("0");
	}
	
	/*
		Called by MenuBansManager
	*/
	void Init(ref MenuBansManager rootClass, BanDuration dateInfo = null)
	{
		m_RootClass = rootClass;
		
		//Setup Edit boxes with data
		if (dateInfo != null)
		{
			m_InputYear.SetText(dateInfo.Year.ToString());
			m_InputMonth.SetText(dateInfo.Month.ToString());
			m_InputDay.SetText(dateInfo.Day.ToString());
			m_InputHour.SetText(dateInfo.Hour.ToString());
			m_InputMinute.SetText(dateInfo.Minute.ToString());
			m_PermaTick.SetChecked(dateInfo.Permanent);
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		switch(w)
		{
			case m_Apply:
			m_RootClass.UpdateBanDuration(new BanDuration(m_InputYear.GetText().ToInt(),m_InputMonth.GetText().ToInt(),m_InputDay.GetText().ToInt(),m_InputHour.GetText().ToInt(),m_InputMinute.GetText().ToInt(),m_PermaTick.IsChecked()));
			delete this;
			return true;
			break;
			
			case m_Close:
			delete this;
			return true;
			break;
			
			case m_brtnYrDec:
			case m_brtnMnthDec:
			case m_brtnDayDec:
			case m_brtnHrDec:
			case m_brtnMinDec:
			AdjustValue(ButtonWidget.Cast(w),true);
			break;
			
			case m_brtnYrInc:
			case m_brtnMnthInc:
			case m_brtnDayInc:
			case m_brtnHrInc:
			case m_brtnMinInc:
			AdjustValue(ButtonWidget.Cast(w),false);
			break;
			
		}
		return false;
	}
	
	void AdjustValue(ButtonWidget w, bool decrement)
	{
		autoptr EditBoxWidget box;
		switch(w)
		{
			case m_brtnYrInc:
			case m_brtnYrDec:
			box = m_InputYear;
			break;
			
			case m_brtnMnthInc:
			case m_brtnMnthDec:
			box = m_InputMonth;
			break;
			
			case m_brtnDayInc:
			case m_brtnDayDec:
			box = m_InputDay;
			break;
			
			case m_brtnHrInc:
			case m_brtnHrDec:
			box = m_InputHour;
			break;
			
			case m_brtnMinInc:
			case m_brtnMinDec:
			box = m_InputMinute;
			break;
		}
		
		int old = box.GetText().ToInt();
		if (old <= 1){
			box.SetText("1");
			return;
		}
	
		if (decrement)
			old--;
		else
			old++;
			
		box.SetText(old.ToString());
	}
};