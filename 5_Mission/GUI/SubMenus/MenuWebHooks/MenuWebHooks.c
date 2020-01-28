class MenuWebHooks extends AdminHudSubMenu
{
	private EditBoxWidget    		  m_urlText;
	private EditBoxWidget    		  m_webHookName;
	private GridSpacerWidget 		  m_gridWebHooks;
	private CheckBoxWidget 			  m_deathKillLogs;
	private CheckBoxWidget			  m_adminActivityLog;
	private CheckBoxWidget 			  m_admHitLog;
	private CheckBoxWidget			  m_joinLeaveLog;
	private ButtonWidget 			  m_btnSaveChanges;
	private ButtonWidget 			  m_btnCreateWebhook;
	private bool 				   	  m_loaded = false;
	private bool 					  createMode;

	private int m_selectedIndex;
	private ref array<ref WebHookEntry>  m_webHookEntries;
	private ref array<ref WebHook> 		 m_webHooksData; //This array is cordinated with m_webHookEntries in terms of index

	void MenuWebHooks()
	{
		m_webHookEntries = new array<ref WebHookEntry>;
		m_webHooksData   = new array<ref WebHook>;
		//------RPCs-------
		GetRPCManager().AddRPC( "RPC_MenuWebHooks", "PopulateList", this);
		//-----------------
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);

		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/WebHooksUI/MenuWebHooks.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		m_urlText     = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "urlText") );
		m_webHookName = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "webHookName") );
		
		m_deathKillLogs = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "deathKillLogs") );
		m_adminActivityLog = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "adminActivityLog") );
		m_admHitLog = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "admHitLog") );
		m_joinLeaveLog = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "joinLeaveLog") );
		m_btnSaveChanges = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "btnSaveChanges") );
		GetVPPUIManager().HookConfirmationDialog(m_btnSaveChanges, M_SUB_WIDGET, this, "SaveEdits", DIAGTYPE.DIAG_YESNO, "Save Changes?", "Are you sure you wish to save changes?");
		
		m_btnCreateWebhook = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "btnCreateWebhook") );

		m_gridWebHooks = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "gridWebHooks") );

		m_btnSaveChanges.Enable( false );
		m_loaded = true;

		//Get Data
		GetRPCManager().SendRPC("RPC_WebHooksManager", "GetWebHooks", null ,true);
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() && !m_loaded) return;
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		return super.OnChange(w, x, y, finished);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		switch(w)
		{
			case m_btnCreateWebhook:
			createMode = true;
			GetVPPUIManager().DisplayNotification("Please Configure your webhook settings on the left then save!");
			m_btnCreateWebhook.Enable( false );
			m_btnSaveChanges.Enable( true );
			return true;
			break;
		}
		return super.OnClick(w, x, y, button);
	}
	
	void SaveEdits(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			if (m_selectedIndex == -1 && !createMode)
			{
				GetVPPUIManager().DisplayNotification("No webhook was selected!");
				return;
			}

			string newName = m_webHookName.GetText();
			if (newName == "" || newName.Length() < 3)
			{
				GetVPPUIManager().DisplayNotification("Error, WebHook name too short! 3 characters minimum");
				return;
			}

			if (createMode && !CheckDuplicate( newName ))
			{
				ref WebHook newWebHook = new WebHook(newName, m_urlText.GetText());
				newWebHook.m_deathKillLogs    = m_deathKillLogs.IsChecked();
				newWebHook.m_adminActivityLog = m_adminActivityLog.IsChecked();
				newWebHook.m_admHitLog 		  = m_admHitLog.IsChecked();
				newWebHook.m_joinLeaveLog 	  = m_joinLeaveLog.IsChecked();
				GetRPCManager().SendRPC("RPC_WebHooksManager", "CreateWebHooks", new Param1<ref WebHook>(newWebHook), true);
				createMode = false;
			}
			else
			{
				ref WebHook data = m_webHooksData.Get( m_selectedIndex );
				string oldName = data.GetName();
				if (data)
				{
					data.SetName(newName);
					data.SetURL(m_urlText.GetText());
		
					data.m_deathKillLogs    = m_deathKillLogs.IsChecked();
					data.m_adminActivityLog = m_adminActivityLog.IsChecked();
					data.m_admHitLog 		= m_admHitLog.IsChecked();
					data.m_joinLeaveLog 	= m_joinLeaveLog.IsChecked();
					GetRPCManager().SendRPC("RPC_WebHooksManager", "EditWebHooks", new Param3<int,string,ref WebHook>(m_selectedIndex, oldName, data), true);
				}
			}
		}
	}

	bool CheckDuplicate(string name)
	{
		foreach(ref WebHook cfg: m_webHooksData)
		{
			if (cfg.GetName() == name && createMode)
			{
				GetVPPUIManager().DisplayNotification("The name: " + name + " is already used!\nPlease use a unique name");
				return true;
			}
		}
		return false;
	}

	void SetSelected(ref WebHookEntry entry)
	{
		int index = m_webHookEntries.Find( entry );
		if (index > -1)
		{
			m_selectedIndex = index;
			GetVPPUIManager().DisplayNotification("Selected Webhook: " + entry.GetName());
			//Update option & fields
			autoptr WebHook dataClass = m_webHooksData.Get( index );
			m_btnSaveChanges.Enable( true );
			m_btnCreateWebhook.Enable( false );

			m_deathKillLogs.SetChecked(dataClass.m_deathKillLogs);
			m_adminActivityLog.SetChecked(dataClass.m_adminActivityLog);
			m_admHitLog.SetChecked(dataClass.m_admHitLog);
			m_joinLeaveLog.SetChecked(dataClass.m_joinLeaveLog);
			m_urlText.SetText( dataClass.GetURL() );
			m_webHookName.SetText( dataClass.GetName() );
		}
	}

	void DeleteWebHook(ref WebHookEntry entry)
	{
		int index = m_webHookEntries.Find( entry );
		if (index > -1)
			GetRPCManager().SendRPC("RPC_WebHooksManager", "DeleteWebHooks", new Param2<int,string>(index, entry.GetName()), true);
		else
			GetVPPUIManager().DisplayNotification("Something went really wrong\nCould not delete webhook: " + entry.GetName());
	}

	void ClearEntries()
	{
		if (m_webHookEntries == null || m_webHookEntries.Count() == 0) return;
		foreach(ref WebHookEntry entry: m_webHookEntries)
		{
			if (entry)
				delete entry;
		}

		m_btnSaveChanges.Enable( false );
		m_btnCreateWebhook.Enable( true );
		m_deathKillLogs.SetChecked( false );
		m_adminActivityLog.SetChecked( false );
		m_admHitLog.SetChecked( false );
		m_joinLeaveLog.SetChecked( false );
		m_urlText.SetText("");
		m_webHookName.SetText("");
		m_webHookEntries = new array<ref WebHookEntry>;
		m_selectedIndex = -1;
	}
	
	void PopulateList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		ref Param1<ref array<ref WebHook>> data;
		if (!ctx.Read(data)) return;
		
		if ( type == CallType.Client )
		{
			m_webHooksData = new array<ref WebHook>;
			m_webHooksData = data.param1;

			ClearEntries();

			foreach(ref WebHook cfg: m_webHooksData)
			{
				m_webHookEntries.Insert( new WebHookEntry(m_gridWebHooks, cfg.GetName()) );
			}
		}
	}
};