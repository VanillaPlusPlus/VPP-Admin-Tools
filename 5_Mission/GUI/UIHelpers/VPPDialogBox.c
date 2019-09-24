/*
Custom DialogBox
*/
enum DIAGTYPE
{
	DIAG_YESNO,
	DIAG_YESNOCANCEL,
	DIAG_OK,
	DIAG_OK_CANCEL_INPUT,
};

enum DIAGRESULT
{
	YES,
	NO,
	OK,
	CANCEL,
};

class VPPDialogBox extends ScriptedWidgetEventHandler
{
	protected Widget 	     m_Root;
	protected ButtonWidget   m_Yes;
	protected ButtonWidget   m_No;
	protected ButtonWidget   m_Ok;
	protected ButtonWidget   m_Cancel;
	protected EditBoxWidget  m_InputBox;
	protected TextWidget     m_TitleText;
	protected RichTextWidget m_Content;
	private string 			 m_ContentText;
	private Class        	 m_CallBackClass;
	private int 			 m_diagType;
	
	void OnWidgetScriptInit(Widget w)
	{
		m_Root = w;
		m_Root.SetHandler(this);
		m_Yes      = ButtonWidget.Cast(m_Root.FindAnyWidget("ButtonYES"));
		m_No       = ButtonWidget.Cast(m_Root.FindAnyWidget("ButtonNO"));
		m_Ok       = ButtonWidget.Cast(m_Root.FindAnyWidget("ButtonOK"));
		m_Cancel   = ButtonWidget.Cast(m_Root.FindAnyWidget("ButtonCANCEL"));
		m_InputBox = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputBox"));
		autoptr EditBoxEventHandler editClass;
		m_InputBox.GetScript(editClass);
		editClass.SetStep(1);
		/* All Buttons are hidden on start */
		m_TitleText = TextWidget.Cast(m_Root.FindAnyWidget("TitleText"));
		m_Content   = RichTextWidget.Cast(m_Root.FindAnyWidget("ContentText"));
		
		m_Root.SetSort(1024,true);
	}
	
	void ~VPPDialogBox()
	{
		/*Destroy*/
		if (m_Root != null)
			m_Root.Unlink();
	}
	
	void InitDiagBox(int diagType, string title, string content, Class callBackClass)
	{
		m_CallBackClass = callBackClass;
		m_diagType = diagType;
		switch(diagType)
		{
			case DIAGTYPE.DIAG_YESNO:
			m_Yes.Show(true);
			m_No.Show(true);
			break;
			
			case DIAGTYPE.DIAG_YESNOCANCEL:
			m_Yes.Show(true);
			m_No.Show(true);
			m_Cancel.Show(true);
			break;
			
			case DIAGTYPE.DIAG_OK:
			m_Ok.Show(true);
			break;
			
			case DIAGTYPE.DIAG_OK_CANCEL_INPUT:
			m_Ok.Show(true);
			m_Cancel.Show(true);
			m_InputBox.Show(true);
			break;
		}
		//Setup Text
		m_TitleText.SetText(title);
		m_Content.SetText(content);
	}
	
	void AllowCharInput()
	{
		autoptr EditBoxEventHandler editClass;
		m_InputBox.GetScript(editClass);
		editClass.AllowCharInput();
	}

	void SetContentText(string text)
	{
		m_Content.SetText(text);
		m_ContentText = text;
	}
	
	string GetContentText()
	{
		return m_ContentText;
	}
	
	string GetInputText()
	{
		return m_InputBox.GetText();
	}
	
	void MakeSmall()
	{
		if (m_Root == null) return;
		float oldX,oldY;
		m_Root.FindAnyWidget("BorderOutline").GetSize(oldX,oldY);
		m_Root.FindAnyWidget("BorderOutline").SetSize(oldX - 0.2, oldY - 0.1,true);
	}
	
	private void OnOutCome(int result)
	{
		GetGame().GameScript.CallFunction( m_CallBackClass, "OnDiagResult", null, result );
		delete this;
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		int result = -1;
		switch(w)
		{
			case m_Yes:
			OnOutCome(DIAGRESULT.YES);
			break;
			
			case m_No:
			OnOutCome(DIAGRESULT.NO);
			break;
			
			case m_Ok:
			if (m_diagType == DIAGTYPE.DIAG_OK_CANCEL_INPUT)
			{
				GetGame().GameScript.CallFunctionParams( m_CallBackClass, "OnDiagResult", null, new Param2<int,string>( DIAGRESULT.OK, GetInputText()));
				delete this;
			}else
				OnOutCome(DIAGRESULT.OK);
			break;
			
			case m_Cancel:
			OnOutCome(DIAGRESULT.CANCEL);
			break;
		}
		return false;
	}
};