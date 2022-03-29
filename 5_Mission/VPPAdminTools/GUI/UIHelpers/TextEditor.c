class VPPTextEditor extends UIScriptedMenu
{
	private Widget 	 		 	   M_WIDGET;
	private ButtonWidget 		   m_Apply;
	private ButtonWidget 		   m_Hide;
	private MultilineEditBoxWidget m_TextInput;
	private ScrollWidget           m_Scroller;
	protected ref Timer            m_WidgetUpdate;
	protected float width,height;
	protected int prevLineCount;
	protected bool IsVisible 	= false;
	private ref AdminHudSubMenu   m_ParentClass;
	
	void VPPTextEditor(Widget parentWidget, AdminHudSubMenu parentClass, array<string> newText = null)
	{
		M_WIDGET    = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.CustomInputMenu, parentWidget.FindAnyWidget("Panel_Content"));
		m_Apply     = ButtonWidget.Cast( M_WIDGET.FindAnyWidget( "BtnApply") );
		m_Hide      = ButtonWidget.Cast( M_WIDGET.FindAnyWidget( "BtnClose") );
		m_TextInput = MultilineEditBoxWidget.Cast( M_WIDGET.FindAnyWidget( "TextInput") );
		m_Scroller  = ScrollWidget.Cast( M_WIDGET.FindAnyWidget( "Scroller") );
		
		m_ParentClass = parentClass;
		
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_Apply, this, "ButtonClick" );
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_Hide, this, "ButtonClick" );
		
		UpdateText(newText);
		ShowEditor(false);
		m_WidgetUpdate = new Timer;
		m_WidgetUpdate.Run(0.1,this,"UpdateScroller",null,true);
	}
	
	void ~VPPTextEditor()
	{
		m_WidgetUpdate.Stop();
		M_WIDGET.Unlink();
	}
	
	void UpdateText(array<string> newText){
		if (newText != null){
			ClearText();
			foreach(string line : newText){
				m_TextInput.SetLine(m_TextInput.GetLinesCount(), line);
			}
		}
	}
	
	void ClearText(){
		if (m_TextInput){
			m_TextInput.SetText("");
		}
	}
	
	bool isEditorVisible(){
		return IsVisible;
	}
	
	void ShowEditor(bool state){
		IsVisible = state;
		M_WIDGET.Show(state);
	}
	
	/*TEMP till they fix MultilineEditBoxWidget auto scroll*/
	void UpdateScroller()
	{
		if (!isEditorVisible()) return;
		
		int lines = m_TextInput.GetLinesCount();
		if (lines >= 35 && lines != prevLineCount)
		{
			m_TextInput.GetSize(width,height);
			m_TextInput.SetSize(width,height + 0.3,true);
			prevLineCount = lines;
		}
		m_TextInput.Update();
	}
	
	void ButtonClick( Widget w, int x, int y, int button )
	{
		switch(w)
		{
			case m_Hide:
				ShowEditor(false);
				m_ParentClass.ShowSubMenu();
			break;
			
			case m_Apply:
				ShowEditor(false);
				m_ParentClass.ShowSubMenu();
			break;
		}
	}
};