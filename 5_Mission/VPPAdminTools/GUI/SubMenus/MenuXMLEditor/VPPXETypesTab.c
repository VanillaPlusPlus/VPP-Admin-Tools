/*
	XML Editor TYPES tab (WP5): inspector of one type (all fields, chips, notes, definitions, spawn info,
	issues, preview, structural ops with override confirmation) and the bulk editor.

	Edits are staged in VPPXEClientModel against a TARGET definition (the DefSelectHost entry: the last
	definition by default, the scoped file's definition in file scope). A definition that lacks an element
	shows what it inherits (PriorMerge of the earlier definitions) with the MIXED chip tint.
*/
class VPPXETypesTab : ScriptedWidgetEventHandler
{
	const static float REFRESH_INTERVAL = 0.25;
	// InspHeader height in layout units (holds the 88-unit item preview); InspScroll starts 2 units below it.
	const static float INSP_HEADER_H = 96.0;
	//the shared dropdown prefab lays out at most 50 rows (dropdown_content GridSpacer 'Rows 50')
	const static int MOVE_DD_MAX_ROWS = 50;

	protected MenuXMLEditor m_Owner;
	protected Widget m_Root;
	protected Widget m_InspectorEmpty;
	protected Widget m_InspectorUnreg;
	protected ItemPreviewWidget m_UnregPreview;
	protected TextWidget m_TxtUnregName;
	protected TextWidget m_TxtUnregGame;
	protected TextWidget m_TxtUnregBody;
	protected TextWidget m_TxtUnregTarget;
	protected ButtonWidget m_BtnUnregRegister;
	protected TextWidget m_TxtUnregRegister;
	protected EntityAI m_UnregObject;
	protected string m_UnregName;
	protected string m_UnregSig;
	protected Widget m_InspectorRoot;
	protected Widget m_InspHeader;
	protected Widget m_InspScroll;
	protected Widget m_InspSpacer;
	protected Widget m_ActionBar;
	protected Widget m_BulkRoot;
	protected Widget m_BulkScroll;
	protected Widget m_BulkSpacer;
	protected Widget m_TitleClip;
	protected ItemPreviewWidget m_ItemPreview;
	protected TextWidget m_TxtTypeName;
	protected TextWidget m_TxtTypeSub;
	protected ImageWidget m_ImgInspHelp;
	protected ButtonWidget m_BtnDuplicate;
	protected ButtonWidget m_BtnMoveType;
	protected ButtonWidget m_BtnRenameType;
	protected ButtonWidget m_BtnDeleteType;
	protected Widget m_DefSelectHost;
	protected Widget m_MoveTargetHost;
	protected ref VPPDropDownMenu m_DefDD;
	protected ref VPPDropDownMenu m_MoveDD;
	protected ref array<string> m_DefFiles;
	protected ref array<string> m_DefLabels;
	protected ref array<string> m_MoveFiles;
	protected ref array<string> m_MoveLabels;
	protected ref array<string> m_MoveAll;
	protected int m_MovePage;
	protected int m_MovePageNext;

	protected ref array<string> m_FieldNames;
	protected ref array<string> m_FieldLabelKeys;
	protected ref array<string> m_FieldHelpKeys;
	protected ref array<int> m_FieldBits;
	protected ref array<EditBoxWidget> m_Inputs;
	protected ref array<VPPXENumericEditHandler> m_Handlers;
	protected ref array<ButtonWidget> m_BtnDecs;
	protected ref array<ButtonWidget> m_BtnIncs;
	protected ref array<TextWidget> m_TxtEffs;
	protected TextWidget m_TxtLifetimeHuman;
	protected TextWidget m_TxtRestockHuman;
	protected ref array<string> m_FlagNames;
	protected ref array<string> m_FlagLabelKeys;
	protected ref array<CheckBoxWidget> m_Checks;
	protected ref VPPXEChipGroup m_ChipsCategory;
	protected ref VPPXEChipGroup m_ChipsUsage;
	protected ref VPPXEChipGroup m_ChipsValue;
	protected ref VPPXEChipGroup m_ChipsTag;
	protected int m_UsagePlain;
	protected int m_ValuePlain;
	protected TextWidget m_TxtUsageNote;
	protected TextWidget m_TxtValueNote;
	protected TextWidget m_TxtTagNote;
	protected TextWidget m_TxtSpawnInfo;
	protected TextWidget m_TxtDefs;
	protected TextWidget m_TxtFoundIn;
	protected ButtonWidget m_BtnOpenSpawnables;
	// the SPAWNABLES data revision the spawn / found-in texts were built from
	protected int m_SpwRev;
	protected TextWidget m_TxtTypeIssues;
	protected TextWidget m_TxtDirty;
	protected ButtonWidget m_BtnRevert;
	protected ButtonWidget m_BtnRevertAll;
	protected ButtonWidget m_BtnSave;
	protected TextWidget m_TxtRevert;
	protected TextWidget m_TxtRevertAll;
	protected TextWidget m_TxtSave;
	protected ref array<ref VPPCollapsibleSection> m_Sections;

	protected TextWidget m_TxtBulkCount;
	protected ButtonWidget m_BtnBulkClose;
	protected ButtonWidget m_BtnBulkApply;
	protected ButtonWidget m_BtnBulkReset;
	protected ref array<CheckBoxWidget> m_BulkChecks;
	protected ref array<ButtonWidget> m_BulkSetBtns;
	protected ref array<ButtonWidget> m_BulkAddBtns;
	protected ref array<ButtonWidget> m_BulkScaleBtns;
	protected ref array<TextWidget> m_BulkSetTxts;
	protected ref array<TextWidget> m_BulkAddTxts;
	protected ref array<TextWidget> m_BulkScaleTxts;
	protected ref array<VPPXENumericEditHandler> m_BulkHandlers;
	protected ref array<int> m_BulkModes;
	protected ref array<ButtonWidget> m_BulkFlagBtns;
	protected ref array<TextWidget> m_BulkFlagTxts;
	protected ref array<int> m_BulkFlagStates;
	protected ref VPPXEChipGroup m_BulkCategory;
	protected ref VPPXEChipGroup m_BulkUsage;
	protected ref VPPXEChipGroup m_BulkValue;
	protected ref VPPXEChipGroup m_BulkTag;
	protected ref array<string> m_BulkNames;
	protected ref array<string> m_BulkTargets;

	protected string m_TypeName;
	protected string m_TargetFile;
	protected ref VPPXETypeDetails m_Details;
	protected bool m_Shown;
	protected bool m_BulkMode;
	protected string m_LimitsSig;
	protected float m_RefreshTimer;
	protected int m_StateSig;
	protected float m_Unit;
	protected float m_LastRootW;
	protected float m_LastRootH;

	protected EntityAI m_PreviewObject;
	protected string m_PreviewName;
	protected int m_RotationX;
	protected int m_RotationY;
	protected vector m_ItemOrientation;

	protected int m_PendOp;
	protected string m_PendName;
	protected string m_PendNewName;
	protected string m_PendSrcFile;
	protected string m_PendTargetFile;
	protected ref map<int, string> m_SelectAfterSave;

	void VPPXETypesTab(Widget host, MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_TypeName = "";
		m_TargetFile = "";
		m_UnregName = "";
		m_UnregSig = "-";
		m_LimitsSig = "-";
		m_StateSig = -1;
		m_Unit = 1.0;
		m_PendOp = -1;
		m_DefFiles = new array<string>();
		m_DefLabels = new array<string>();
		m_MoveFiles = new array<string>();
		m_MoveLabels = new array<string>();
		m_MoveAll = new array<string>();
		m_MovePageNext = -1;
		m_FieldNames = {"Nominal", "Min", "Lifetime", "Restock", "Cost", "QuantMin", "QuantMax"};
		m_FieldLabelKeys = {"#VSTR_XMLE_F_NOMINAL", "#VSTR_XMLE_F_MIN", "#VSTR_XMLE_F_LIFETIME", "#VSTR_XMLE_F_RESTOCK", "#VSTR_XMLE_F_COST", "#VSTR_XMLE_F_QUANTMIN", "#VSTR_XMLE_F_QUANTMAX"};
		m_FieldHelpKeys = {"#VSTR_XMLE_HELP_NOMINAL", "#VSTR_XMLE_HELP_MIN", "#VSTR_XMLE_HELP_LIFETIME", "#VSTR_XMLE_HELP_RESTOCK", "#VSTR_XMLE_HELP_COST", "#VSTR_XMLE_HELP_QUANTMIN", "#VSTR_XMLE_HELP_QUANTMAX"};
		m_FieldBits = new array<int>();
		m_FieldBits.Insert(VPPXEField.NOMINAL);
		m_FieldBits.Insert(VPPXEField.MIN);
		m_FieldBits.Insert(VPPXEField.LIFETIME);
		m_FieldBits.Insert(VPPXEField.RESTOCK);
		m_FieldBits.Insert(VPPXEField.COST);
		m_FieldBits.Insert(VPPXEField.QUANTMIN);
		m_FieldBits.Insert(VPPXEField.QUANTMAX);
		m_FlagNames = {"Cargo", "Hoarder", "Map", "Player", "Crafted", "Deloot"};
		m_FlagLabelKeys = {"#VSTR_XMLE_FLAG_CARGO", "#VSTR_XMLE_FLAG_HOARDER", "#VSTR_XMLE_FLAG_MAP", "#VSTR_XMLE_FLAG_PLAYER", "#VSTR_XMLE_FLAG_CRAFTED", "#VSTR_XMLE_FLAG_DELOOT"};
		m_Inputs = new array<EditBoxWidget>();
		m_Handlers = new array<VPPXENumericEditHandler>();
		m_BtnDecs = new array<ButtonWidget>();
		m_BtnIncs = new array<ButtonWidget>();
		m_TxtEffs = new array<TextWidget>();
		m_Checks = new array<CheckBoxWidget>();
		m_Sections = new array<ref VPPCollapsibleSection>();
		m_BulkChecks = new array<CheckBoxWidget>();
		m_BulkSetBtns = new array<ButtonWidget>();
		m_BulkAddBtns = new array<ButtonWidget>();
		m_BulkScaleBtns = new array<ButtonWidget>();
		m_BulkSetTxts = new array<TextWidget>();
		m_BulkAddTxts = new array<TextWidget>();
		m_BulkScaleTxts = new array<TextWidget>();
		m_BulkHandlers = new array<VPPXENumericEditHandler>();
		m_BulkModes = new array<int>();
		m_BulkFlagBtns = new array<ButtonWidget>();
		m_BulkFlagTxts = new array<TextWidget>();
		m_BulkFlagStates = new array<int>();
		m_BulkNames = new array<string>();
		m_BulkTargets = new array<string>();
		m_SelectAfterSave = new map<int, string>();

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorTypesTab, host);
		m_Root.SetHandler(this);
		BindWidgets();
		SetupInputs();
		SetupSections();
		SetupTooltips();

		m_ChipsCategory = new VPPXEChipGroup(m_Root.FindAnyWidget("ChipsCategory"), true, false);
		m_ChipsUsage = new VPPXEChipGroup(m_Root.FindAnyWidget("ChipsUsage"), false, false);
		m_ChipsValue = new VPPXEChipGroup(m_Root.FindAnyWidget("ChipsValue"), false, false);
		m_ChipsTag = new VPPXEChipGroup(m_Root.FindAnyWidget("ChipsTag"), false, false);
		m_ChipsCategory.m_OnChanged.Insert(OnCategoryChip);
		m_ChipsUsage.m_OnChanged.Insert(OnUsageChip);
		m_ChipsValue.m_OnChanged.Insert(OnValueChip);
		m_ChipsTag.m_OnChanged.Insert(OnTagChip);
		m_BulkCategory = new VPPXEChipGroup(m_Root.FindAnyWidget("ChipsBulkCategory"), true, false);
		m_BulkUsage = new VPPXEChipGroup(m_Root.FindAnyWidget("ChipsBulkUsage"), false, true);
		m_BulkValue = new VPPXEChipGroup(m_Root.FindAnyWidget("ChipsBulkValue"), false, true);
		m_BulkTag = new VPPXEChipGroup(m_Root.FindAnyWidget("ChipsBulkTag"), false, true);

		m_DefDD = new VPPDropDownMenu(m_DefSelectHost, "-");
		m_DefDD.m_OnSelectItem.Insert(OnDefSelected);
		m_MoveDD = new VPPDropDownMenu(m_MoveTargetHost, "-");
		m_MoveDD.m_OnSelectItem.Insert(OnMoveTargetSelected);
		m_MoveTargetHost.Show(false);

		ResetBulk();
		ShowEmpty();
	}

	void ~VPPXETypesTab()
	{
		ReleasePreview();
		ReleaseUnregPreview();
	}

	protected void BindWidgets()
	{
		m_InspectorEmpty = m_Root.FindAnyWidget("InspectorEmpty");
		m_InspectorUnreg = m_Root.FindAnyWidget("InspectorUnreg");
		m_UnregPreview = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("UnregPreview"));
		m_TxtUnregName = TextWidget.Cast(m_Root.FindAnyWidget("TxtUnregName"));
		m_TxtUnregGame = TextWidget.Cast(m_Root.FindAnyWidget("TxtUnregGame"));
		m_TxtUnregBody = TextWidget.Cast(m_Root.FindAnyWidget("TxtUnregBody"));
		m_TxtUnregTarget = TextWidget.Cast(m_Root.FindAnyWidget("TxtUnregTarget"));
		m_BtnUnregRegister = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnUnregRegister"));
		m_TxtUnregRegister = TextWidget.Cast(m_Root.FindAnyWidget("TxtUnregRegister"));
		m_InspectorRoot = m_Root.FindAnyWidget("InspectorRoot");
		m_InspHeader = m_Root.FindAnyWidget("InspHeader");
		m_InspScroll = m_Root.FindAnyWidget("InspScroll");
		m_InspSpacer = m_Root.FindAnyWidget("InspSpacer");
		m_ActionBar = m_Root.FindAnyWidget("ActionBar");
		m_BulkRoot = m_Root.FindAnyWidget("BulkRoot");
		m_BulkScroll = m_Root.FindAnyWidget("BulkScroll");
		m_BulkSpacer = m_Root.FindAnyWidget("BulkSpacer");
		m_TitleClip = m_Root.FindAnyWidget("InspTitleClip");
		m_ItemPreview = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("ItemPreview"));
		m_TxtTypeName = TextWidget.Cast(m_Root.FindAnyWidget("TxtTypeName"));
		m_TxtTypeSub = TextWidget.Cast(m_Root.FindAnyWidget("TxtTypeSub"));
		m_ImgInspHelp = ImageWidget.Cast(m_Root.FindAnyWidget("ImgInspHelp"));
		m_BtnDuplicate = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnDuplicate"));
		m_BtnMoveType = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMoveType"));
		m_BtnRenameType = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnRenameType"));
		m_BtnDeleteType = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnDeleteType"));
		m_DefSelectHost = m_Root.FindAnyWidget("DefSelectHost");
		m_MoveTargetHost = m_Root.FindAnyWidget("MoveTargetHost");
		m_TxtLifetimeHuman = TextWidget.Cast(m_Root.FindAnyWidget("TxtLifetimeHuman"));
		m_TxtRestockHuman = TextWidget.Cast(m_Root.FindAnyWidget("TxtRestockHuman"));
		m_TxtUsageNote = TextWidget.Cast(m_Root.FindAnyWidget("TxtUsageNote"));
		m_TxtValueNote = TextWidget.Cast(m_Root.FindAnyWidget("TxtValueNote"));
		m_TxtTagNote = TextWidget.Cast(m_Root.FindAnyWidget("TxtTagNote"));
		m_TxtSpawnInfo = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpawnInfo"));
		m_TxtDefs = TextWidget.Cast(m_Root.FindAnyWidget("TxtDefs"));
		m_TxtFoundIn = TextWidget.Cast(m_Root.FindAnyWidget("TxtFoundIn"));
		m_BtnOpenSpawnables = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnOpenSpawnables"));
		m_TxtTypeIssues = TextWidget.Cast(m_Root.FindAnyWidget("TxtTypeIssues"));
		m_TxtDirty = TextWidget.Cast(m_Root.FindAnyWidget("TxtDirty"));
		m_BtnRevert = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnRevert"));
		m_BtnRevertAll = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnRevertAll"));
		m_BtnSave = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSave"));
		m_TxtRevert = TextWidget.Cast(m_Root.FindAnyWidget("TxtRevert"));
		m_TxtRevertAll = TextWidget.Cast(m_Root.FindAnyWidget("TxtRevertAll"));
		m_TxtSave = TextWidget.Cast(m_Root.FindAnyWidget("TxtSave"));
		m_TxtBulkCount = TextWidget.Cast(m_Root.FindAnyWidget("TxtBulkCount"));
		m_BtnBulkClose = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBulkClose"));
		m_BtnBulkApply = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBulkApply"));
		m_BtnBulkReset = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBulkReset"));

		for (int i = 0; i < m_FieldNames.Count(); i++)
		{
			string field = m_FieldNames[i];
			EditBoxWidget box = EditBoxWidget.Cast(m_Root.FindAnyWidget("Input" + field));
			VPPXENumericEditHandler handler = null;
			if (box)
			{
				box.GetScript(handler);
			}

			m_Inputs.Insert(box);
			m_Handlers.Insert(handler);
			m_BtnDecs.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnDec" + field)));
			m_BtnIncs.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnInc" + field)));
			m_TxtEffs.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtEff" + field)));

			EditBoxWidget bulkBox = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputBulk" + field));
			VPPXENumericEditHandler bulkHandler = null;
			if (bulkBox)
			{
				bulkBox.GetScript(bulkHandler);
			}

			m_BulkHandlers.Insert(bulkHandler);
			m_BulkChecks.Insert(CheckBoxWidget.Cast(m_Root.FindAnyWidget("ChkBulk" + field)));
			m_BulkSetBtns.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBulkSet" + field)));
			m_BulkAddBtns.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBulkAdd" + field)));
			m_BulkScaleBtns.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBulkScale" + field)));
			m_BulkSetTxts.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtBulkSet" + field)));
			m_BulkAddTxts.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtBulkAdd" + field)));
			m_BulkScaleTxts.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtBulkScale" + field)));
			m_BulkModes.Insert(0);
		}

		for (int j = 0; j < m_FlagNames.Count(); j++)
		{
			string flagName = m_FlagNames[j];
			m_Checks.Insert(CheckBoxWidget.Cast(m_Root.FindAnyWidget("Chk" + flagName)));
			m_BulkFlagBtns.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBulkFlag" + flagName)));
			m_BulkFlagTxts.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtBulkFlag" + flagName)));
			m_BulkFlagStates.Insert(0);
		}
	}

	protected void SetupInputs()
	{
		for (int i = 0; i < m_Handlers.Count(); i++)
		{
			int bit = m_FieldBits[i];
			VPPXENumericEditHandler handler = m_Handlers[i];
			if (handler)
			{
				handler.SetRange(FieldMin(bit), FieldMax(bit));
				handler.SetStep(FieldStep(bit));
				handler.m_OnValueChanged.Insert(OnInputChanged);
			}

			VPPXENumericEditHandler bulkHandler = m_BulkHandlers[i];
			if (bulkHandler)
			{
				//full 9-digit span so SET can reach any value the inspector accepts; ClampField clamps each result
				bulkHandler.SetRange(-999999999, 999999999);
				bulkHandler.SetStep(FieldStep(bit));
			}
		}
	}

	protected void SetupSections()
	{
		array<string> sectionNames = {"Economy", "Flags", "Category", "Usage", "Value", "Tag", "Spawn", "FoundIn", "Defs", "Issues"};
		for (int i = 0; i < sectionNames.Count(); i++)
		{
			string sec = sectionNames[i];
			m_Sections.Insert(new VPPCollapsibleSection(m_Root, "Sec" + sec + "Header", "Sec" + sec + "Content", "Sec" + sec + "Chevron"));
		}
	}

	protected void SetupTooltips()
	{
		ToolTipHandler helpTip;
		if (m_ImgInspHelp)
		{
			m_ImgInspHelp.GetScript(helpTip);
		}

		if (helpTip)
		{
			helpTip.SetTitle("#VSTR_TOOLTIP_TITLE");
			helpTip.SetContentText("#VSTR_XMLE_TOOLTIP_INSPECTOR");
		}

		for (int i = 0; i < m_FieldNames.Count(); i++)
		{
			Widget info = m_Root.FindAnyWidget("Info" + m_FieldNames[i]);
			ToolTipHandler fieldTip = null;
			if (info)
			{
				info.GetScript(fieldTip);
			}

			if (fieldTip)
			{
				fieldTip.SetTitle(m_FieldLabelKeys[i]);
				fieldTip.SetContentText(m_FieldHelpKeys[i]);
			}
		}
	}

	//---------------------------------------------------------------- field rules

	protected bool IsQuant(int bit)
	{
		return bit == VPPXEField.QUANTMIN || bit == VPPXEField.QUANTMAX;
	}

	protected int FieldMin(int bit)
	{
		if (IsQuant(bit))
		{
			return -1;
		}

		return 0;
	}

	protected int FieldMax(int bit)
	{
		if (IsQuant(bit))
		{
			return 100;
		}

		return 999999999;
	}

	protected int FieldStep(int bit)
	{
		if (bit == VPPXEField.LIFETIME)
		{
			return 600;
		}

		if (bit == VPPXEField.RESTOCK)
		{
			return 60;
		}

		if (bit == VPPXEField.COST)
		{
			return 10;
		}

		if (IsQuant(bit))
		{
			return 5;
		}

		return 1;
	}

	protected int ClampField(int bit, int num)
	{
		int low = FieldMin(bit);
		int high = FieldMax(bit);
		if (num < low)
		{
			return low;
		}

		if (num > high)
		{
			return high;
		}

		return num;
	}

	//---------------------------------------------------------------- owner helpers

	protected VPPXEClientModel Model()
	{
		return m_Owner.GetModel();
	}

	protected string Tr(string keyOrText)
	{
		return m_Owner.Tr(keyOrText);
	}

	protected bool HasPerm(int permBit)
	{
		return m_Owner.HasPerm(permBit);
	}

	//true when the target definition can be edited now (permission, EDITABLE, no save in flight)
	bool CanEditTarget()
	{
		if (m_TypeName == "" || m_TargetFile == "")
		{
			return false;
		}

		if (!HasPerm(VPPXEPerm.EDIT_TYPES))
		{
			return false;
		}

		VPPXEClientModel model = Model();
		if (!model.IsEditable(m_TargetFile) || model.IsFileLocked(m_TargetFile))
		{
			return false;
		}

		return model.DefExists(m_TargetFile, m_TypeName);
	}

	//ADD target: the scoped file (only when EDITABLE) or, in merged scope, the first EDITABLE TYPES file
	string AddTargetFile()
	{
		VPPXEClientModel model = Model();
		string scope = m_Owner.GetTypesScope();
		if (scope != "")
		{
			if (model.IsEditable(scope))
			{
				return scope;
			}

			return "";
		}

		return model.FirstEditableTypesFile();
	}

	//---------------------------------------------------------------- tab contract

	void Show(bool show)
	{
		m_Shown = show;
		m_Root.Show(show);
		if (show)
		{
			OnResize();
			RefreshState(true);
		}
	}

	void Refresh()
	{
		RefreshState(true);
	}

	void OnResize()
	{
		float rootW;
		float rootH;
		m_Root.GetScreenSize(rootW, rootH);
		if (rootW <= 0 || rootH <= 0)
		{
			return;
		}

		m_LastRootW = rootW;
		m_LastRootH = rootH;

		//screen pixels -> layout units (Main is "scaled 1", so units are not pixels)
		UpdateUnit();
		float rootUnitsW = rootW / m_Unit;
		float rootUnitsH = rootH / m_Unit;
		float inspH = rootUnitsH - INSP_HEADER_H - 2 - 38;
		if (inspH < 60)
		{
			inspH = 60;
		}

		m_InspScroll.SetSize(1, inspH);
		float bulkH = rootUnitsH - 34 - 38;
		if (bulkH < 60)
		{
			bulkH = 60;
		}

		m_BulkScroll.SetSize(1, bulkH);
		float clipW = rootUnitsW - 100 - 332;
		if (clipW < 60)
		{
			clipW = 60;
		}

		m_TitleClip.SetSize(clipW, INSP_HEADER_H);
		LayoutActionBar();
		RenderDetailsTexts();
		m_InspSpacer.Update();
		m_BulkSpacer.Update();
	}

	//screen pixels per layout unit, measured on the exact INSP_HEADER_H-unit InspHeader (30-unit BulkHeader when that is not laid out); keeps the last good value
	protected void UpdateUnit()
	{
		float headerW;
		float headerH;
		if (m_InspHeader)
		{
			m_InspHeader.GetScreenSize(headerW, headerH);
			if (headerH > 1)
			{
				m_Unit = headerH / INSP_HEADER_H;
				return;
			}
		}

		Widget bulkHeader = m_Root.FindAnyWidget("BulkHeader");
		if (bulkHeader)
		{
			bulkHeader.GetScreenSize(headerW, headerH);
			if (headerH > 1)
			{
				m_Unit = headerH / 30;
			}
		}
	}

	protected float LabelWidth(string key, float minW, float maxW)
	{
		string label = Tr(key);
		return Math.Clamp(30 + 7 * label.LengthUtf8(), minW, maxW);
	}

	protected void LayoutActionBar()
	{
		float saveW = LabelWidth("#VSTR_XMLE_BTN_SAVE", 70, 150);
		float allW = LabelWidth("#VSTR_XMLE_BTN_REVERT_ALL", 70, 150);
		float revertW = LabelWidth("#VSTR_XMLE_BTN_REVERT", 70, 150) + 18;
		if (revertW > 150)
		{
			revertW = 150;
		}

		m_BtnSave.SetPos(6, 0);
		m_BtnSave.SetSize(saveW + 18, 26);
		m_BtnRevertAll.SetPos(6 + saveW + 18 + 6, 0);
		m_BtnRevertAll.SetSize(allW, 26);
		m_BtnRevert.SetPos(6 + saveW + 18 + 6 + allW + 6, 0);
		m_BtnRevert.SetSize(revertW, 26);
	}

	//a rail toggle can change the root size after the single ResizeActiveTab read a stale width
	protected void CheckRootResize()
	{
		if (!m_Root)
		{
			return;
		}

		float rootW;
		float rootH;
		m_Root.GetScreenSize(rootW, rootH);
		if (rootW <= 0 || rootH <= 0)
		{
			return;
		}

		if (rootW != m_LastRootW || rootH != m_LastRootH)
		{
			OnResize();
		}
	}

	void OnUpdate(float timeslice)
	{
		if (!m_Shown)
		{
			return;
		}

		CheckRootResize();
		ApplyMovePageNext();
		CheckSpawnablesRevision();
		m_RefreshTimer += timeslice;
		if (m_RefreshTimer >= REFRESH_INTERVAL)
		{
			m_RefreshTimer = 0;
			RefreshState(false);
		}
	}

	void OnSessionChanged()
	{
		RebuildChipItems();
		if (m_TypeName != "")
		{
			Render();
		}

		RefreshState(true);
	}

	//called by the menu after an index reply replaced rows
	void OnRowsChanged()
	{
		if (m_TypeName == "")
		{
			return;
		}

		VPPXEClientModel model = Model();
		if (m_TargetFile == "" || !model.DefExists(m_TargetFile, m_TypeName))
		{
			ChooseTarget();
		}

		BuildDefDropdown();
		Render();
		RenderDetailsTexts();
	}

	void OnScopeChanged()
	{
		if (m_TypeName == "")
		{
			RefreshState(true);
			return;
		}

		ChooseTarget();
		BuildDefDropdown();
		Render();
		RenderDetailsTexts();
	}

	//the rail filter or search changed: keep the bulk count current
	void OnFilterChanged()
	{
		if (m_BulkMode)
		{
			UpdateBulkCount();
		}
	}

	//a save result for one of our uploads arrived
	void OnSaveFinished(int reqId, bool ok)
	{
		string selectName;
		if (!m_SelectAfterSave.Find(reqId, selectName))
		{
			RefreshState(true);
			return;
		}

		m_SelectAfterSave.Remove(reqId);
		if (ok && selectName != "")
		{
			m_Owner.SelectType(selectName, false);
		}

		RefreshState(true);
	}

	//---------------------------------------------------------------- type display

	protected void ShowEmpty()
	{
		m_BulkMode = false;
		HideUnregistered();
		m_BulkRoot.Show(false);
		m_InspectorRoot.Show(false);
		m_InspectorEmpty.Show(true);
	}

	protected void ShowInspector()
	{
		m_BulkMode = false;
		HideUnregistered();
		m_BulkRoot.Show(false);
		m_InspectorEmpty.Show(false);
		m_InspectorRoot.Show(true);
	}

	//---------------------------------------------------------------- not registered (config class without a types entry)

	// The rail's "not registered" filter picked a game config class that no types file defines: explain that and offer
	// REGISTER, which adds a template <type> to the ADD target file and then opens it in the inspector.
	void ShowUnregistered(string className)
	{
		m_TypeName = "";
		m_TargetFile = "";
		m_Details = null;
		CancelMovePick();
		m_BulkMode = false;
		m_BulkRoot.Show(false);
		m_InspectorRoot.Show(false);
		m_InspectorEmpty.Show(false);
		m_UnregName = className;
		m_UnregSig = "-";
		m_TxtUnregName.SetText(className);
		string gameName = VPPXEConfigClasses.GameNameOf(className);
		if (gameName == "" || gameName == className)
		{
			m_TxtUnregGame.SetText("");
		}
		else
		{
			m_TxtUnregGame.SetText(gameName);
		}

		string rootName = VPPXEConfigClasses.RootOf(className);
		if (rootName == "")
		{
			rootName = "CfgVehicles";
		}

		m_TxtUnregBody.SetText(string.Format(Tr("#VSTR_XMLE_UNREG_BODY"), rootName));
		m_InspectorUnreg.Show(true);
		UpdateUnregPreview();
		RefreshUnregistered(true);
		RefreshState(true);
	}

	protected void HideUnregistered()
	{
		if (m_UnregName == "")
		{
			return;
		}

		m_UnregName = "";
		m_UnregSig = "-";
		m_InspectorUnreg.Show(false);
		ReleaseUnregPreview();
	}

	// REGISTER needs AddRemoveTypes + EditTypes, an EDITABLE target file and no save in flight on it.
	protected void RefreshUnregistered(bool force)
	{
		if (m_UnregName == "")
		{
			return;
		}

		VPPXEClientModel model = Model();
		bool permOk = HasPerm(VPPXEPerm.ADD_REMOVE) && HasPerm(VPPXEPerm.EDIT_TYPES);
		string target = AddTargetFile();
		bool locked = target != "" && model.IsFileLocked(target);
		string sig = target;
		if (permOk)
		{
			sig = sig + "|p";
		}

		if (locked)
		{
			sig = sig + "|l";
		}

		if (!force && sig == m_UnregSig)
		{
			return;
		}

		m_UnregSig = sig;
		bool allowed = permOk && target != "" && !locked;
		if (!permOk)
		{
			m_TxtUnregTarget.SetText(Tr("#VSTR_XMLE_UNREG_NO_PERM"));
		}
		else if (target == "")
		{
			m_TxtUnregTarget.SetText(Tr("#VSTR_XMLE_UNREG_NO_TARGET"));
		}
		else
		{
			m_TxtUnregTarget.SetText(string.Format(Tr("#VSTR_XMLE_UNREG_TARGET"), model.FileLabel(target)));
		}

		m_BtnUnregRegister.Enable(allowed);
		if (allowed)
		{
			m_TxtUnregRegister.SetColor(ARGB(255, 255, 255, 255));
		}
		else
		{
			m_TxtUnregRegister.SetColor(ARGB(255, 92, 97, 102));
		}
	}

	protected void BeginRegister()
	{
		if (m_UnregName == "" || !HasPerm(VPPXEPerm.ADD_REMOVE) || !HasPerm(VPPXEPerm.EDIT_TYPES))
		{
			return;
		}

		VPPXEClientModel model = Model();
		string target = AddTargetFile();
		if (target == "")
		{
			m_Owner.NotifyError("#VSTR_XMLE_ERR_READONLY_FILE");
			return;
		}

		if (model.IsFileLocked(target))
		{
			return;
		}

		ClearPending();
		m_PendOp = VPPXEOp.ADD;
		m_PendName = m_UnregName;
		m_PendSrcFile = target;
		m_PendTargetFile = target;
		string body = string.Format(Tr("#VSTR_XMLE_UNREG_DLG_BODY"), m_UnregName, model.FileLabel(target)) + PendingNote(target, "");
		m_Owner.OpenConfirm("#VSTR_XMLE_UNREG_DLG_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmRegister", false);
	}

	void OnConfirmRegister(int result, string input)
	{
		if (result != DIAGRESULT.YES || m_PendOp != VPPXEOp.ADD || m_PendName == "")
		{
			ClearPending();
			return;
		}

		string newName = m_PendName;
		if (!ValidateResultName(newName, m_PendTargetFile))
		{
			ClearPending();
			return;
		}

		array<string> others = new array<string>();
		Model().OtherDefFiles(newName, m_PendTargetFile, "", others);
		if (others.Count() > 0)
		{
			AskOverride(newName, others, Model().DescribeEffectChange(newName, VPPXEOp.ADD, "", "", m_PendTargetFile));
			return;
		}

		SendStructural(false);
		RefreshUnregistered(true);
	}

	//local preview object of the unregistered class (the scan already left out AI and buildings)
	protected void UpdateUnregPreview()
	{
		ReleaseUnregPreview();
		if (m_UnregName == "" || !m_UnregPreview)
		{
			return;
		}

		Object created = GetGame().CreateObject(m_UnregName, vector.Zero, true, false, false);
		m_UnregObject = EntityAI.Cast(created);
		if (!m_UnregObject)
		{
			if (created)
			{
				GetGame().ObjectDelete(created);
			}

			return;
		}

		m_UnregPreview.SetItem(m_UnregObject);
		m_UnregPreview.SetModelPosition(Vector(0, 0, 0.5));
		m_UnregPreview.SetModelOrientation(Vector(0, 0, 0));
		m_UnregPreview.SetView(m_UnregObject.GetViewIndex());
		m_UnregPreview.Show(true);
	}

	protected void ReleaseUnregPreview()
	{
		if (m_UnregPreview)
		{
			m_UnregPreview.SetItem(null);
			m_UnregPreview.Show(false);
		}

		if (m_UnregObject && GetGame())
		{
			GetGame().ObjectDelete(m_UnregObject);
		}

		m_UnregObject = null;
	}

	void ShowType(string typeName)
	{
		m_TypeName = typeName;
		m_Details = null;
		CancelMovePick();
		ChooseTarget();
		BuildDefDropdown();
		ShowInspector();
		Render();
		RenderDetailsTexts();
		UpdatePreview();
		OnResize();
	}

	void ShowDetails(VPPXETypeDetails d)
	{
		if (!d)
		{
			return;
		}

		m_Details = d;
		RenderDetailsTexts();
		UpdatePreview();
	}

	protected void ChooseTarget()
	{
		VPPXEClientModel model = Model();
		string scope = m_Owner.GetTypesScope();
		if (scope != "" && model.DefExists(scope, m_TypeName))
		{
			m_TargetFile = scope;
			return;
		}

		m_TargetFile = model.LastDefFile(m_TypeName);
	}

	//file name part (after the last /) shortened from the left with "..." to at most budget characters
	protected string FitFileName(string fileKey, int budget)
	{
		string fileName = fileKey;
		int slash = fileKey.LastIndexOf("/");
		if (slash >= 0)
		{
			fileName = fileKey.Substring(slash + 1, fileKey.Length() - slash - 1);
		}

		if (budget < 4)
		{
			budget = 4;
		}

		if (fileName.Length() > budget)
		{
			int keep = budget - 3;
			fileName = "..." + fileName.Substring(fileName.Length() - keep, keep);
		}

		return fileName;
	}

	//whole file key (folder kept, so several '<mod>/types.xml' stay distinct) shortened from the left with "..." to at most budget characters
	protected string FitFileKey(string fileKey, int budget)
	{
		if (budget < 4)
		{
			budget = 4;
		}

		if (fileKey.Length() <= budget)
		{
			return fileKey;
		}

		int keep = budget - 3;
		return "..." + fileKey.Substring(fileKey.Length() - keep, keep);
	}

	//dropdown entry fitted into 21 characters: file name (shortened from the left) + " - " + status word (always kept)
	protected string FitDefEntry(string fileKey, string statusKey)
	{
		string status = Tr(statusKey);
		string fileName = FitFileName(fileKey, 21 - 3 - status.LengthUtf8());
		return fileName + " - " + status;
	}

	protected void BuildDefDropdown()
	{
		VPPXEClientModel model = Model();
		m_DefFiles.Clear();
		m_DefLabels.Clear();
		m_DefDD.RemoveAllElements();
		if (m_TypeName != "")
		{
			model.GetDefFiles(m_TypeName, m_DefFiles);
		}

		int selected = -1;
		for (int i = 0; i < m_DefFiles.Count(); i++)
		{
			string statusKey = "#VSTR_XMLE_DEF_OVERRIDDEN";
			if (i == m_DefFiles.Count() - 1)
			{
				statusKey = "#VSTR_XMLE_DEF_WINS";
			}

			string entry = FitDefEntry(m_DefFiles[i], statusKey);
			m_DefLabels.Insert(entry);
			m_DefDD.AddElement(entry);
			if (m_DefFiles[i] == m_TargetFile)
			{
				selected = i;
			}
		}

		if (selected >= 0)
		{
			m_DefDD.SetText(m_DefLabels[selected]);
			m_DefDD.SetIndex(selected);
		}
		else
		{
			m_DefDD.SetText("-");
			m_DefDD.SetIndex(-1);
		}
	}

	void OnDefSelected(int index)
	{
		if (index < 0 || index >= m_DefFiles.Count())
		{
			m_DefDD.Close();
			return;
		}

		m_DefDD.SetText(m_DefLabels[index]);
		m_DefDD.SetIndex(index);
		m_DefDD.Close();
		m_TargetFile = m_DefFiles[index];
		Render();
		RenderDetailsTexts();
	}

	//full render of the inspector values for (m_TypeName, m_TargetFile) with staged edits
	void Render()
	{
		if (m_TypeName == "")
		{
			return;
		}

		VPPXEClientModel model = Model();
		m_TxtTypeName.SetText(model.DisplayNameOf(m_TypeName));
		if (m_TargetFile != "")
		{
			m_TxtTypeSub.SetText(string.Format(Tr("#VSTR_XMLE_EDITING_FMT"), m_TargetFile));
		}
		else
		{
			m_TxtTypeSub.SetText("");
		}

		VPPXETypeRow shown = null;
		VPPXETypeRow prior = null;
		if (m_TargetFile != "")
		{
			shown = model.GetDisplayRow(m_TypeName, m_TargetFile);
			prior = model.PriorMerge(m_TypeName, m_TargetFile);
		}
		else
		{
			prior = VPPXETypeMerge.NewRow(m_TypeName);
		}

		RenderScalars(shown, -1);
		RenderFlags(shown, prior);
		RenderChips(shown, prior);
		RenderNotes(shown);
		RenderDirty();
		RefreshState(true);
		m_InspSpacer.Update();
	}

	protected void RenderScalars(VPPXETypeRow shown, int skipIdx)
	{
		for (int i = 0; i < m_FieldBits.Count(); i++)
		{
			int bit = m_FieldBits[i];
			VPPXENumericEditHandler handler = m_Handlers[i];
			if (handler && i != skipIdx)
			{
				if (shown && (shown.Present & bit) != 0)
				{
					handler.SetValue(VPPXETypeMerge.GetScalar(shown, bit));
				}
				else
				{
					handler.SetEmpty();
				}
			}

			RenderEffective(i);
		}

		RenderHuman(m_TxtLifetimeHuman, 2);
		RenderHuman(m_TxtRestockHuman, 3);
	}

	protected void RenderEffective(int idx)
	{
		TextWidget eff = m_TxtEffs[idx];
		if (!eff)
		{
			return;
		}

		int num;
		string winner;
		bool found = Model().DisplayScalar(m_TypeName, "", m_FieldBits[idx], num, winner);
		string effText = "";
		if (!found)
		{
			effText = Tr("#VSTR_XMLE_UNSET");
		}
		else if (winner != m_TargetFile)
		{
			effText = string.Format(Tr("#VSTR_XMLE_EFFECTIVE"), num, Model().FileLabel(winner));
		}

		eff.SetText(effText);
	}

	protected void RenderHuman(TextWidget human, int idx)
	{
		if (!human)
		{
			return;
		}

		int seconds = -1;
		VPPXENumericEditHandler handler = m_Handlers[idx];
		if (handler && handler.HasValue())
		{
			seconds = handler.GetValue();
		}
		else
		{
			int num;
			string winner;
			if (Model().DisplayScalar(m_TypeName, "", m_FieldBits[idx], num, winner))
			{
				seconds = num;
			}
		}

		human.SetText(FormatDuration(seconds));
	}

	//seconds -> "2d 3h" style text with the translated unit keys ("" when negative)
	protected string FormatDuration(int seconds)
	{
		if (seconds < 0)
		{
			return "";
		}

		int days = seconds / 86400;
		int hours = (seconds % 86400) / 3600;
		int mins = (seconds % 3600) / 60;
		int secs = seconds % 60;
		string unitD = Tr("#VSTR_XMLE_UNIT_D");
		string unitH = Tr("#VSTR_XMLE_UNIT_H");
		string unitM = Tr("#VSTR_XMLE_UNIT_M");
		string unitS = Tr("#VSTR_XMLE_UNIT_S");
		string result = "";
		if (days > 0)
		{
			result = days.ToString() + unitD;
			if (hours > 0)
			{
				result = result + " " + hours.ToString() + unitH;
			}

			return result;
		}

		if (hours > 0)
		{
			result = hours.ToString() + unitH;
			if (mins > 0)
			{
				result = result + " " + mins.ToString() + unitM;
			}

			return result;
		}

		if (mins > 0)
		{
			result = mins.ToString() + unitM;
			if (secs > 0)
			{
				result = result + " " + secs.ToString() + unitS;
			}

			return result;
		}

		return secs.ToString() + unitS;
	}

	protected void RenderFlags(VPPXETypeRow shown, VPPXETypeRow prior)
	{
		int flags = 0;
		if (shown && (shown.Present & VPPXEField.FLAGS) != 0)
		{
			flags = shown.Flags;
		}
		else if (prior && (prior.Present & VPPXEField.FLAGS) != 0)
		{
			flags = prior.Flags;
		}

		for (int i = 0; i < m_Checks.Count(); i++)
		{
			CheckBoxWidget chk = m_Checks[i];
			if (chk)
			{
				int flagBit = VPPXETypeMerge.FlagBitAt(i);
				chk.SetChecked((flags & flagBit) != 0);
			}
		}
	}

	protected void RenderChips(VPPXETypeRow shown, VPPXETypeRow prior)
	{
		VPPXEClientModel model = Model();
		m_ChipsCategory.SetAll(VPPXEChipGroup.CHIP_OFF);
		int catIdx = 0;
		int catState = VPPXEChipGroup.CHIP_ON;
		if (shown && (shown.Present & VPPXEField.CATEGORY) != 0)
		{
			if (shown.Category >= 0)
			{
				catIdx = shown.Category + 1;
			}
		}
		else if (prior && (prior.Present & VPPXEField.CATEGORY) != 0 && prior.Category >= 0)
		{
			catIdx = prior.Category + 1;
			catState = VPPXEChipGroup.CHIP_MIXED;
		}

		m_ChipsCategory.SetState(catIdx, catState);

		if (model.DefinesElement(shown, VPPXEField.USAGE))
		{
			m_ChipsUsage.SetMask(shown.Usage, shown.UsageUser, m_UsagePlain, VPPXEChipGroup.CHIP_ON);
		}
		else if (prior)
		{
			m_ChipsUsage.SetMask(prior.Usage, prior.UsageUser, m_UsagePlain, VPPXEChipGroup.CHIP_MIXED);
		}
		else
		{
			m_ChipsUsage.SetAll(VPPXEChipGroup.CHIP_OFF);
		}

		if (model.DefinesElement(shown, VPPXEField.VALUE))
		{
			m_ChipsValue.SetMask(shown.Value, shown.ValueUser, m_ValuePlain, VPPXEChipGroup.CHIP_ON);
		}
		else if (prior)
		{
			m_ChipsValue.SetMask(prior.Value, prior.ValueUser, m_ValuePlain, VPPXEChipGroup.CHIP_MIXED);
		}
		else
		{
			m_ChipsValue.SetAll(VPPXEChipGroup.CHIP_OFF);
		}

		int tagCount = m_ChipsTag.Count();
		if (model.DefinesElement(shown, VPPXEField.TAG))
		{
			m_ChipsTag.SetMask(shown.Tag, 0, tagCount, VPPXEChipGroup.CHIP_ON);
		}
		else if (prior)
		{
			m_ChipsTag.SetMask(prior.Tag, 0, tagCount, VPPXEChipGroup.CHIP_MIXED);
		}
		else
		{
			m_ChipsTag.SetAll(VPPXEChipGroup.CHIP_OFF);
		}
	}

	//NOTE_NO_CLEAR: the edited list of a non-first definition would be empty, so the engine keeps an earlier list
	protected void RenderNotes(VPPXETypeRow shown)
	{
		RenderNote(m_TxtUsageNote, shown, VPPXEField.USAGE);
		RenderNote(m_TxtValueNote, shown, VPPXEField.VALUE);
		RenderNote(m_TxtTagNote, shown, VPPXEField.TAG);
	}

	protected void RenderNote(TextWidget note, VPPXETypeRow shown, int elementBit)
	{
		if (!note)
		{
			return;
		}

		VPPXEClientModel model = Model();
		string noteText = "";
		VPPXEStagedEdit staged = null;
		if (m_TargetFile != "")
		{
			staged = model.GetStaged(m_TargetFile, m_TypeName);
		}

		if (staged && staged.TouchesElement(elementBit) && !model.DefinesElement(shown, elementBit))
		{
			string earlier = model.PriorElementFile(m_TypeName, m_TargetFile, elementBit);
			if (earlier != "")
			{
				noteText = string.Format(Tr("#VSTR_XMLE_NOTE_NO_CLEAR"), model.FileLabel(earlier));
			}
		}

		SetMultiline(note, noteText);
		note.Show(noteText != "");
	}

	protected void RenderDirty()
	{
		int count = Model().CountStaged();
		if (count > 0)
		{
			m_TxtDirty.SetText(string.Format(Tr("#VSTR_XMLE_DIRTY_FMT"), count));
		}
		else
		{
			m_TxtDirty.SetText(Tr("#VSTR_XMLE_DIRTY_NONE"));
		}
	}

	//spawn info, definitions and issues (details arrive after ShowType)
	protected void RenderDetailsTexts()
	{
		if (m_TypeName == "")
		{
			return;
		}

		VPPXEClientModel model = Model();
		string loading = Tr("#VSTR_XMLE_STATUS_LOADING");

		//definitions come from the model rows (Line of the last occurrence per file)
		array<string> defFiles = new array<string>();
		model.GetDefFiles(m_TypeName, defFiles);
		array<string> defLines = new array<string>();
		for (int i = 0; i < defFiles.Count(); i++)
		{
			VPPXETypeRow defRow = model.GetDefRow(defFiles[i], m_TypeName);
			string lineText = "-";
			if (defRow)
			{
				lineText = defRow.Line.ToString();
			}

			string statusKey = "#VSTR_XMLE_DEF_OVERRIDDEN";
			if (i == defFiles.Count() - 1)
			{
				statusKey = "#VSTR_XMLE_DEF_WINS";
			}

			defLines.Insert(string.Format(Tr("#VSTR_XMLE_DEF_LINE"), model.FileLabel(defFiles[i]), lineText, Tr(statusKey)));
		}

		SetMultiline(m_TxtDefs, JoinLines(defLines));
		bool clientSpawnables = RenderSpawnables();

		if (!m_Details)
		{
			if (!clientSpawnables)
			{
				SetMultiline(m_TxtSpawnInfo, loading);
				SetMultiline(m_TxtFoundIn, loading);
			}

			SetMultiline(m_TxtTypeIssues, loading);
			m_InspSpacer.Update();
			return;
		}

		array<string> spawnLines = new array<string>();
		VPPXESpawnableInfo spawnable = m_Details.Spawnable;
		if (!spawnable || !spawnable.Found)
		{
			spawnLines.Insert(Tr("#VSTR_XMLE_SPAWN_NONE"));
		}
		else
		{
			if (spawnable.HasDamage)
			{
				spawnLines.Insert(string.Format(Tr("#VSTR_XMLE_SPAWN_DAMAGE"), spawnable.DamageMin, spawnable.DamageMax));
			}

			if (spawnable.Hoarder)
			{
				spawnLines.Insert(Tr("#VSTR_XMLE_SPAWN_HOARDER"));
			}

			if (spawnable.Parents && spawnable.Parents.Count() > 0)
			{
				spawnLines.Insert(string.Format(Tr("#VSTR_XMLE_SPAWN_PARENTS"), JoinWith(spawnable.Parents, ", ")));
			}

			if (spawnable.Lines)
			{
				for (int j = 0; j < spawnable.Lines.Count(); j++)
				{
					spawnLines.Insert(spawnable.Lines[j]);
				}
			}
		}

		// the server's summary of the saved files until the SPAWNABLES rows are loaded
		if (!clientSpawnables)
		{
			SetMultiline(m_TxtSpawnInfo, JoinLines(spawnLines));
			SetMultiline(m_TxtFoundIn, loading);
		}

		array<string> issueLines = new array<string>();
		if (m_Details.Issues)
		{
			for (int k = 0; k < m_Details.Issues.Count(); k++)
			{
				VPPXEIssue issue = m_Details.Issues[k];
				if (!issue)
				{
					continue;
				}

				string issueKey = VPPXEText.IssueKey(issue.Code);
				if (issueKey == "")
				{
					continue;
				}

				issueLines.Insert(string.Format(Tr(issueKey), issue.Arg));
			}
		}

		if (issueLines.Count() == 0)
		{
			issueLines.Insert(Tr("#VSTR_XMLE_NO_ISSUES"));
		}

		if (m_Details.ConfigState == VPPXEConfigState.MISSING)
		{
			issueLines.Insert(Tr("#VSTR_XMLE_CONFIG_MISSING"));
		}

		VPPXEStagedEdit staged = null;
		if (m_TargetFile != "")
		{
			staged = model.GetStaged(m_TargetFile, m_TypeName);
		}

		if (staged && staged.Conflict)
		{
			issueLines.Insert(Tr("#VSTR_XMLE_CONFLICT_NOTE"));
		}

		SetMultiline(m_TxtTypeIssues, JoinLines(issueLines));
		m_InspSpacer.Update();
	}

	// "Spawns with" / "found in" from the SPAWNABLES tab's rows (with their unsaved edits, the server's roll rules);
	// false while those load (asked for here, even with the tab hidden).
	protected bool RenderSpawnables()
	{
		VPPXESpawnablesTab spawnables = null;
		if (m_Owner)
		{
			spawnables = m_Owner.GetSpawnablesTab();
		}

		if (!spawnables)
		{
			return false;
		}

		m_SpwRev = spawnables.DataRevision();
		spawnables.EnsureLoaded();
		if (!spawnables.IsDataLoaded())
		{
			return false;
		}

		array<string> spawnsWith = new array<string>();
		spawnables.DescribeSpawnsWith(m_TypeName, spawnsWith);
		string spawnsText = JoinLines(spawnsWith);
		SetMultiline(m_TxtSpawnInfo, spawnsText);
		array<string> foundIn = new array<string>();
		spawnables.DescribeFoundIn(m_TypeName, foundIn);
		string foundText = JoinLines(foundIn);
		SetMultiline(m_TxtFoundIn, foundText);
		return true;
	}

	// The SPAWNABLES rows changed (loaded, edited, saved): rebuild the texts of the shown type.
	protected void CheckSpawnablesRevision()
	{
		if (m_TypeName == "" || !m_Owner)
		{
			return;
		}

		VPPXESpawnablesTab spawnables = m_Owner.GetSpawnablesTab();
		if (spawnables && spawnables.DataRevision() != m_SpwRev)
		{
			RenderDetailsTexts();
		}
	}

	protected string JoinLines(array<string> lines)
	{
		return JoinWith(lines, "\n");
	}

	protected string JoinWith(array<string> parts, string separator)
	{
		if (!parts || parts.Count() == 0)
		{
			return "";
		}

		string joined = parts[0];
		for (int i = 1; i < parts.Count(); i++)
		{
			joined = joined + separator + parts[i];
		}

		return joined;
	}

	//sets a multiline text and sizes its height from an estimate of the wrapped line count (12 px text)
	protected void SetMultiline(TextWidget txt, string content)
	{
		if (!txt)
		{
			return;
		}

		txt.SetText(content);
		float width;
		float height;
		txt.GetScreenSize(width, height);
		if (width <= 0)
		{
			width = 400;
		}
		else
		{
			width = width / m_Unit;
		}

		int perLine = width / 6.5;
		if (perLine < 10)
		{
			perLine = 10;
		}

		int lines = 0;
		int start = 0;
		int len = content.Length();
		while (start <= len)
		{
			int stop = -1;
			if (start < len)
			{
				stop = content.IndexOfFrom(start, "\n");
			}

			if (stop < 0)
			{
				stop = len;
			}

			//byte offsets drive IndexOfFrom; the wrap estimate counts characters
			int segment = 0;
			if (stop > start)
			{
				string lineText = content.Substring(start, stop - start);
				segment = lineText.LengthUtf8();
			}

			int wrapped = 1;
			if (segment > perLine)
			{
				wrapped = (segment + perLine - 1) / perLine;
			}

			lines = lines + wrapped;
			start = stop + 1;
		}

		if (lines < 1)
		{
			lines = 1;
		}

		txt.SetSize(0.98, lines * 16 + 4);
	}

	//---------------------------------------------------------------- chips vocabulary

	protected string LimitsSignature()
	{
		VPPXELimits lim = Model().GetLimits();
		string sig = lim.Categories.Count().ToString() + ":" + lim.Usages.Count().ToString() + ":" + lim.UsageGroups.Count().ToString() + ":" + lim.Values.Count().ToString() + ":" + lim.ValueGroups.Count().ToString() + ":" + lim.Tags.Count().ToString();
		sig = sig + "|" + JoinWith(lim.Categories, ",") + "|" + JoinWith(lim.Usages, ",") + "|" + JoinWith(lim.UsageGroups, ",");
		sig = sig + "|" + JoinWith(lim.Values, ",") + "|" + JoinWith(lim.ValueGroups, ",") + "|" + JoinWith(lim.Tags, ",");
		return sig;
	}

	protected void AppendNames(array<string> outLabels, array<string> names, string prefix, int cap)
	{
		if (!names)
		{
			return;
		}

		for (int i = 0; i < names.Count() && i < cap; i++)
		{
			outLabels.Insert(prefix + names[i]);
		}
	}

	protected void RebuildChipItems()
	{
		string sig = LimitsSignature();
		if (sig == m_LimitsSig)
		{
			return;
		}

		m_LimitsSig = sig;
		VPPXELimits lim = Model().GetLimits();
		m_UsagePlain = Math.Min(lim.Usages.Count(), 32);
		m_ValuePlain = Math.Min(lim.Values.Count(), 32);

		array<string> catLabels = new array<string>();
		catLabels.Insert(Tr("#VSTR_XMLE_CHIP_NONE"));
		AppendNames(catLabels, lim.Categories, "", 1000);
		m_ChipsCategory.SetItems(catLabels);

		array<string> usageLabels = new array<string>();
		AppendNames(usageLabels, lim.Usages, "", 32);
		AppendNames(usageLabels, lim.UsageGroups, "@", 32);
		m_ChipsUsage.SetItems(usageLabels);
		m_BulkUsage.SetItems(usageLabels);

		array<string> valueLabels = new array<string>();
		AppendNames(valueLabels, lim.Values, "", 32);
		AppendNames(valueLabels, lim.ValueGroups, "@", 32);
		m_ChipsValue.SetItems(valueLabels);
		m_BulkValue.SetItems(valueLabels);

		array<string> tagLabels = new array<string>();
		AppendNames(tagLabels, lim.Tags, "", 32);
		m_ChipsTag.SetItems(tagLabels);
		m_BulkTag.SetItems(tagLabels);

		array<string> bulkCatLabels = new array<string>();
		bulkCatLabels.Insert(Tr("#VSTR_XMLE_BULK_KEEP"));
		bulkCatLabels.Insert(Tr("#VSTR_XMLE_CHIP_NONE"));
		AppendNames(bulkCatLabels, lim.Categories, "", 1000);
		m_BulkCategory.SetItems(bulkCatLabels);
		m_BulkCategory.SetState(0, VPPXEChipGroup.CHIP_ON);

		m_InspSpacer.Update();
		m_BulkSpacer.Update();
	}

	//chip index -> (list kind, limits name) of the usage or value group
	protected int ChipListKind(int idx, int plainCount, int plainKind, int groupKind)
	{
		if (idx < plainCount)
		{
			return plainKind;
		}

		return groupKind;
	}

	protected string ChipName(int idx, int plainCount, array<string> plainNames, array<string> groupNames)
	{
		if (idx < plainCount)
		{
			if (idx < plainNames.Count())
			{
				return plainNames[idx];
			}

			return "";
		}

		int groupIdx = idx - plainCount;
		if (groupIdx < groupNames.Count())
		{
			return groupNames[groupIdx];
		}

		return "";
	}

	//---------------------------------------------------------------- enabled state

	protected bool HasOtherEditableFile(string exceptKey)
	{
		VPPXEClientModel model = Model();
		array<string> files = model.GetTypesFiles();
		for (int i = 0; i < files.Count(); i++)
		{
			string key = files[i];
			if (key != exceptKey && model.IsEditable(key) && !model.IsFileLocked(key))
			{
				return true;
			}
		}

		return false;
	}

	//applies the enabled state of every control; force = also when the computed state did not change
	void RefreshState(bool force)
	{
		RefreshUnregistered(force);
		VPPXEClientModel model = Model();
		bool canEdit = CanEditTarget();
		bool editPerm = HasPerm(VPPXEPerm.EDIT_TYPES);
		bool addRemove = HasPerm(VPPXEPerm.ADD_REMOVE);
		bool structOk = addRemove && canEdit;
		bool moveOk = structOk && HasOtherEditableFile(m_TargetFile);
		bool dupInFile = TargetDupInFile();
		bool targetLocked = m_TargetFile != "" && model.IsFileLocked(m_TargetFile);
		bool revertOk = false;
		if (m_TargetFile != "" && !targetLocked && model.GetStaged(m_TargetFile, m_TypeName))
		{
			revertOk = true;
		}

		bool anyStaged = model.CountStaged() > 0;
		bool anyLocked = model.IsAnyLocked();
		bool revertAllOk = anyStaged && !anyLocked;
		bool saveOk = anyStaged && !anyLocked && editPerm;

		int sig = 0;
		if (canEdit)
		{
			sig = sig | 1;
		}

		if (addRemove)
		{
			sig = sig | 2;
		}

		if (structOk)
		{
			sig = sig | 4;
		}

		if (moveOk)
		{
			sig = sig | 8;
		}

		if (revertOk)
		{
			sig = sig | 16;
		}

		if (revertAllOk)
		{
			sig = sig | 32;
		}

		if (saveOk)
		{
			sig = sig | 64;
		}

		if (editPerm)
		{
			sig = sig | 128;
		}

		if (dupInFile)
		{
			sig = sig | 256;
		}

		if (!force && sig == m_StateSig)
		{
			return;
		}

		m_StateSig = sig;
		for (int i = 0; i < m_Handlers.Count(); i++)
		{
			VPPXENumericEditHandler handler = m_Handlers[i];
			if (handler)
			{
				handler.SetEnabled(canEdit);
			}

			ButtonWidget dec = m_BtnDecs[i];
			if (dec)
			{
				dec.Enable(canEdit);
			}

			ButtonWidget inc = m_BtnIncs[i];
			if (inc)
			{
				inc.Enable(canEdit);
			}
		}

		for (int j = 0; j < m_Checks.Count(); j++)
		{
			CheckBoxWidget chk = m_Checks[j];
			if (chk)
			{
				chk.Enable(canEdit);
			}
		}

		m_ChipsCategory.Enable(canEdit);
		m_ChipsUsage.Enable(canEdit);
		m_ChipsValue.Enable(canEdit);
		m_ChipsTag.Enable(canEdit);

		m_BtnDuplicate.Show(addRemove);
		m_BtnMoveType.Show(addRemove);
		m_BtnRenameType.Show(addRemove);
		m_BtnDeleteType.Show(addRemove);
		m_BtnDuplicate.Enable(structOk && !dupInFile);
		m_BtnMoveType.Enable(moveOk && !dupInFile);
		m_BtnRenameType.Enable(structOk);
		m_BtnDeleteType.Enable(structOk);

		m_ActionBar.Show(editPerm);
		m_BtnRevert.Enable(revertOk);
		m_BtnRevertAll.Enable(revertAllOk);
		m_BtnSave.Enable(saveOk);
		m_BtnBulkApply.Enable(editPerm);
		RenderDirty();
	}

	//---------------------------------------------------------------- staging from the inspector

	protected void AfterScalarStaging(int skipIdx)
	{
		VPPXETypeRow shown = null;
		if (m_TargetFile != "")
		{
			shown = Model().GetDisplayRow(m_TypeName, m_TargetFile);
		}

		RenderScalars(shown, skipIdx);
		RenderDirty();
		m_Owner.OnStagingChanged(m_TypeName);
		RefreshState(true);
	}

	protected void AfterFullStaging()
	{
		Render();
		RenderDetailsTexts();
		m_Owner.OnStagingChanged(m_TypeName);
	}

	void OnInputChanged(EditBoxWidget box)
	{
		int idx = m_Inputs.Find(box);
		if (idx < 0)
		{
			return;
		}

		if (!CanEditTarget())
		{
			return;
		}

		VPPXENumericEditHandler handler = m_Handlers[idx];
		int bit = m_FieldBits[idx];
		if (handler.HasValue())
		{
			Model().StageScalar(m_TargetFile, m_TypeName, bit, ClampField(bit, handler.GetValue()));
		}
		else
		{
			Model().StageClearScalar(m_TargetFile, m_TypeName, bit);
		}

		AfterScalarStaging(idx);
	}

	protected void StepField(int idx, int dir)
	{
		if (!CanEditTarget())
		{
			return;
		}

		VPPXENumericEditHandler handler = m_Handlers[idx];
		if (!handler)
		{
			return;
		}

		int bit = m_FieldBits[idx];
		int num = 0;
		if (handler.HasValue())
		{
			num = handler.GetValue();
		}
		else
		{
			int effNum;
			string winner;
			if (Model().DisplayScalar(m_TypeName, "", bit, effNum, winner))
			{
				num = effNum;
			}
		}

		if (IsQuant(bit) && num == -1 && dir > 0)
		{
			num = 0;
		}
		else if (IsQuant(bit) && num == 0 && dir < 0)
		{
			num = -1;
		}
		else
		{
			num = num + dir * FieldStep(bit);
		}

		num = ClampField(bit, num);
		handler.SetValue(num);
		Model().StageScalar(m_TargetFile, m_TypeName, bit, num);
		AfterScalarStaging(-1);
	}

	protected void OnFlagClicked(int idx)
	{
		CheckBoxWidget chk = m_Checks[idx];
		if (!chk || !CanEditTarget())
		{
			Render();
			return;
		}

		Model().StageFlag(m_TargetFile, m_TypeName, VPPXETypeMerge.FlagBitAt(idx), chk.IsChecked());
		AfterFullStaging();
	}

	void OnCategoryChip(int idx, int state)
	{
		if (!CanEditTarget())
		{
			Render();
			return;
		}

		VPPXELimits lim = Model().GetLimits();
		string categoryName = "";
		if (idx > 0 && idx - 1 < lim.Categories.Count())
		{
			categoryName = lim.Categories[idx - 1];
		}

		Model().StageCategory(m_TargetFile, m_TypeName, categoryName);
		AfterFullStaging();
	}

	protected void StageChip(int listKind, string itemName, int state)
	{
		if (!CanEditTarget() || itemName == "")
		{
			Render();
			return;
		}

		bool on = state != VPPXEChipGroup.CHIP_OFF;
		Model().StageListName(m_TargetFile, m_TypeName, listKind, itemName, on);
		AfterFullStaging();
	}

	void OnUsageChip(int idx, int state)
	{
		VPPXELimits lim = Model().GetLimits();
		int kind = ChipListKind(idx, m_UsagePlain, VPPXEClientModel.LIST_USAGE, VPPXEClientModel.LIST_USAGE_USER);
		StageChip(kind, ChipName(idx, m_UsagePlain, lim.Usages, lim.UsageGroups), state);
	}

	void OnValueChip(int idx, int state)
	{
		VPPXELimits lim = Model().GetLimits();
		int kind = ChipListKind(idx, m_ValuePlain, VPPXEClientModel.LIST_VALUE, VPPXEClientModel.LIST_VALUE_USER);
		StageChip(kind, ChipName(idx, m_ValuePlain, lim.Values, lim.ValueGroups), state);
	}

	void OnTagChip(int idx, int state)
	{
		VPPXELimits lim = Model().GetLimits();
		string tagName = "";
		if (idx >= 0 && idx < lim.Tags.Count())
		{
			tagName = lim.Tags[idx];
		}

		StageChip(VPPXEClientModel.LIST_TAG, tagName, state);
	}

	//---------------------------------------------------------------- save / revert

	protected void NotifyOrphans()
	{
		VPPXEClientModel model = Model();
		map<string, int> orphans = model.GetLastOrphans();
		for (int i = 0; i < orphans.Count(); i++)
		{
			string fileKey = orphans.GetKey(i);
			int count = orphans.GetElement(i);
			if (count > 0)
			{
				m_Owner.Notify(string.Format(Tr("#VSTR_XMLE_NOTIFY_ORPHANED"), count, model.FileLabel(fileKey)));
			}
		}
	}

	protected void NotifyTooMany()
	{
		VPPXEClientModel model = Model();
		string fileKey = model.GetLastTooManyFile();
		if (fileKey == "")
		{
			return;
		}

		m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_BULK_TOO_MANY"), model.GetLastTooManyCount(), model.FileLabel(fileKey), VPPXEConst.MAX_EDITS_PER_BATCH));
	}

	protected void OnSaveClicked()
	{
		VPPXEClientModel model = Model();
		int count = model.CountStaged();
		if (count == 0 || model.IsAnyLocked())
		{
			return;
		}

		string body = string.Format(Tr("#VSTR_XMLE_DLG_SAVE_BODY"), count, model.CountFilesWithStaged());
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_SAVE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmSave", false);
	}

	void OnConfirmSave(int result, string input)
	{
		if (result != DIAGRESULT.YES)
		{
			return;
		}

		VPPXEClientModel model = Model();
		if (model.IsAnyLocked() || !HasPerm(VPPXEPerm.EDIT_TYPES))
		{
			return;
		}

		array<ref VPPXEUploadBatch> batches = new array<ref VPPXEUploadBatch>();
		bool built = model.BuildBatches(VPPXEBackupReason.EDIT, batches);
		NotifyOrphans();
		if (!built)
		{
			NotifyTooMany();
			return;
		}

		for (int i = 0; i < batches.Count(); i++)
		{
			VPPXEUploadBatch up = batches[i];
			model.LockFile(up.FileKey, up.ReqId);
			model.RegisterSave(up.ReqId, up.FileKey);
			m_Owner.QueueUpload(up);
		}

		if (batches.Count() > 0)
		{
			m_Owner.SetStatus("#VSTR_XMLE_STATUS_SAVING");
		}

		Render();
		m_Owner.OnStagingChanged("");
	}

	protected void OnRevertClicked()
	{
		VPPXEClientModel model = Model();
		if (m_TargetFile == "" || model.IsFileLocked(m_TargetFile))
		{
			return;
		}

		model.RevertType(m_TargetFile, m_TypeName);
		AfterFullStaging();
	}

	protected void OnRevertAllClicked()
	{
		VPPXEClientModel model = Model();
		int count = model.CountStaged();
		if (count == 0 || model.IsAnyLocked())
		{
			return;
		}

		string body = string.Format(Tr("#VSTR_XMLE_DLG_REVERT_BODY"), count);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_REVERT_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmRevertAll", false);
	}

	void OnConfirmRevertAll(int result, string input)
	{
		if (result != DIAGRESULT.YES)
		{
			return;
		}

		VPPXEClientModel model = Model();
		if (model.IsAnyLocked())
		{
			return;
		}

		model.RevertAll();
		if (m_TypeName != "")
		{
			Render();
			RenderDetailsTexts();
		}

		m_Owner.OnStagingChanged("");
	}

	//---------------------------------------------------------------- structural ops

	protected void ClearPending()
	{
		m_PendOp = -1;
		m_PendName = "";
		m_PendNewName = "";
		m_PendSrcFile = "";
		m_PendTargetFile = "";
	}

	//" <n unsaved change(s) in this file are saved with this action>" when the file has staged edits of other names
	protected string PendingNote(string fileKey, string excludeName)
	{
		VPPXEClientModel model = Model();
		int count = model.CountStagedInFile(fileKey);
		if (excludeName != "" && model.GetStaged(fileKey, excludeName))
		{
			count--;
		}

		if (count <= 0)
		{
			return "";
		}

		return " " + string.Format(Tr("#VSTR_XMLE_DLG_PENDING_NOTE"), count);
	}

	protected string OtherDefsSentence(string typeName, string fileKey)
	{
		array<string> others = new array<string>();
		Model().OtherDefFiles(typeName, fileKey, "", others);
		if (others.Count() == 0)
		{
			return "";
		}

		return string.Format(Tr("#VSTR_XMLE_DLG_OTHER_DEFS"), others.Count());
	}

	protected string LabelsOf(array<string> keys)
	{
		VPPXEClientModel model = Model();
		array<string> labels = new array<string>();
		for (int i = 0; i < keys.Count(); i++)
		{
			labels.Insert(model.FileLabel(keys[i]));
		}

		return JoinWith(labels, ", ");
	}

	//the structural buttons act on the target definition
	protected bool StructPrecheck()
	{
		if (!HasPerm(VPPXEPerm.ADD_REMOVE))
		{
			return false;
		}

		return CanEditTarget();
	}

	//the target name is defined more than once in its own file; the server refuses COPY/MOVE/DUPLICATE of it
	protected bool TargetDupInFile()
	{
		if (m_TargetFile == "" || m_TypeName == "")
		{
			return false;
		}

		VPPXETypeRow dupRow = Model().GetDefRow(m_TargetFile, m_TypeName);
		if (!dupRow)
		{
			return false;
		}

		return (dupRow.Issues & VPPXERowIssue.DUPLICATE) != 0;
	}

	//the RESULTING name must be a valid class name that does not exist in the receiving file
	protected bool ValidateResultName(string resultName, string receivingFile)
	{
		if (!VPPXmlText.IsValidClassName(resultName))
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_ERR_NAME_INVALID"), resultName));
			return false;
		}

		if (Model().DefExists(receivingFile, resultName))
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_ERR_NAME_EXISTS"), resultName));
			return false;
		}

		return true;
	}

	protected void AskOverride(string resultName, array<string> others, string effect)
	{
		string body = string.Format(Tr("#VSTR_XMLE_DLG_OVERRIDE_BODY"), resultName, LabelsOf(others), effect);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_OVERRIDE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmOverride", false);
	}

	void OnConfirmOverride(int result, string input)
	{
		if (result != DIAGRESULT.YES)
		{
			ClearPending();
			return;
		}

		SendStructural(true);
	}

	void BeginAdd()
	{
		if (!HasPerm(VPPXEPerm.ADD_REMOVE) || !HasPerm(VPPXEPerm.EDIT_TYPES))
		{
			return;
		}

		VPPXEClientModel model = Model();
		string target = AddTargetFile();
		if (target == "")
		{
			m_Owner.NotifyError("#VSTR_XMLE_ERR_READONLY_FILE");
			return;
		}

		if (model.IsFileLocked(target))
		{
			return;
		}

		ClearPending();
		m_PendOp = VPPXEOp.ADD;
		m_PendSrcFile = target;
		m_PendTargetFile = target;
		string body = string.Format(Tr("#VSTR_XMLE_DLG_ADD_BODY"), model.FileLabel(target)) + PendingNote(target, "");
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_ADD_TITLE", body, DIAGTYPE.DIAG_OK_CANCEL_INPUT, this, "OnAddName", true);
	}

	void OnAddName(int result, string input)
	{
		if (result != DIAGRESULT.OK || m_PendOp != VPPXEOp.ADD)
		{
			ClearPending();
			return;
		}

		string newName = VPPXmlText.TrimWs(input);
		if (!ValidateResultName(newName, m_PendTargetFile))
		{
			ClearPending();
			return;
		}

		m_PendName = newName;
		array<string> others = new array<string>();
		Model().OtherDefFiles(newName, m_PendTargetFile, "", others);
		if (others.Count() > 0)
		{
			AskOverride(newName, others, Model().DescribeEffectChange(newName, VPPXEOp.ADD, "", "", m_PendTargetFile));
			return;
		}

		SendStructural(false);
	}

	protected void OnDuplicateClicked()
	{
		if (!StructPrecheck() || TargetDupInFile())
		{
			return;
		}

		VPPXEClientModel model = Model();
		if (model.GetStaged(m_TargetFile, m_TypeName))
		{
			m_Owner.NotifyError("#VSTR_XMLE_STRUCT_SAVE_FIRST");
			return;
		}

		ClearPending();
		m_PendOp = VPPXEOp.DUPLICATE;
		m_PendName = model.DisplayNameOf(m_TypeName);
		m_PendSrcFile = m_TargetFile;
		string body = string.Format(Tr("#VSTR_XMLE_DLG_DUP_BODY"), m_PendName) + PendingNote(m_TargetFile, m_TypeName);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_DUP_TITLE", body, DIAGTYPE.DIAG_OK_CANCEL_INPUT, this, "OnNewNameEntered", true);
	}

	protected void OnRenameClicked()
	{
		if (!StructPrecheck())
		{
			return;
		}

		VPPXEClientModel model = Model();
		if (model.GetStaged(m_TargetFile, m_TypeName))
		{
			m_Owner.NotifyError("#VSTR_XMLE_STRUCT_SAVE_FIRST");
			return;
		}

		ClearPending();
		m_PendOp = VPPXEOp.RENAME;
		m_PendName = model.DisplayNameOf(m_TypeName);
		m_PendSrcFile = m_TargetFile;
		string body = string.Format(Tr("#VSTR_XMLE_DLG_RENAME_BODY"), m_PendName, OtherDefsSentence(m_TypeName, m_TargetFile)) + PendingNote(m_TargetFile, m_TypeName);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_RENAME_TITLE", body, DIAGTYPE.DIAG_OK_CANCEL_INPUT, this, "OnNewNameEntered", true);
	}

	//input dialog of DUPLICATE and RENAME
	void OnNewNameEntered(int result, string input)
	{
		if (result != DIAGRESULT.OK || (m_PendOp != VPPXEOp.DUPLICATE && m_PendOp != VPPXEOp.RENAME))
		{
			ClearPending();
			return;
		}

		string newName = VPPXmlText.TrimWs(input);
		if (!ValidateResultName(newName, m_PendSrcFile))
		{
			ClearPending();
			return;
		}

		m_PendNewName = newName;
		array<string> others = new array<string>();
		Model().OtherDefFiles(newName, m_PendSrcFile, "", others);
		if (others.Count() > 0)
		{
			AskOverride(newName, others, Model().DescribeEffectChange(newName, m_PendOp, m_PendName, m_PendSrcFile, m_PendSrcFile));
			return;
		}

		SendStructural(false);
	}

	protected void CancelMovePick()
	{
		if (m_MoveDD)
		{
			m_MoveDD.Close();
		}

		if (m_MoveTargetHost)
		{
			m_MoveTargetHost.Show(false);
		}

		if (m_DefSelectHost)
		{
			m_DefSelectHost.Show(true);
		}
	}

	protected void OnMoveClicked()
	{
		if (m_MoveTargetHost.IsVisible())
		{
			CancelMovePick();
			ClearPending();
			return;
		}

		if (!StructPrecheck() || TargetDupInFile())
		{
			return;
		}

		VPPXEClientModel model = Model();
		if (model.GetStaged(m_TargetFile, m_TypeName))
		{
			m_Owner.NotifyError("#VSTR_XMLE_STRUCT_SAVE_FIRST");
			return;
		}

		//candidates in the source's folder first, then the rest (both in load order)
		string srcFolder = "";
		int srcSlash = m_TargetFile.LastIndexOf("/");
		if (srcSlash >= 0)
		{
			srcFolder = m_TargetFile.Substring(0, srcSlash + 1);
		}

		array<string> sameFolder = new array<string>();
		array<string> otherFolders = new array<string>();
		array<string> files = model.GetTypesFiles();
		for (int i = 0; i < files.Count(); i++)
		{
			string key = files[i];
			if (key == m_TargetFile || !model.IsEditable(key) || model.IsFileLocked(key))
			{
				continue;
			}

			string keyFolder = "";
			int keySlash = key.LastIndexOf("/");
			if (keySlash >= 0)
			{
				keyFolder = key.Substring(0, keySlash + 1);
			}

			if (keyFolder == srcFolder)
			{
				sameFolder.Insert(key);
			}
			else
			{
				otherFolders.Insert(key);
			}
		}

		m_MoveAll.Clear();
		m_MoveAll.InsertAll(sameFolder);
		m_MoveAll.InsertAll(otherFolders);
		if (m_MoveAll.Count() == 0)
		{
			m_Owner.NotifyError("#VSTR_XMLE_ERR_READONLY_FILE");
			return;
		}

		ClearPending();
		m_PendOp = VPPXEOp.MOVE;
		m_PendName = model.DisplayNameOf(m_TypeName);
		m_PendSrcFile = m_TargetFile;
		m_MovePageNext = -1;
		FillMovePage(0);
		m_MoveDD.SetText(Tr("#VSTR_XMLE_DLG_MOVE_TITLE"));
		m_MoveDD.SetIndex(-1);
		m_DefSelectHost.Show(false);
		m_MoveTargetHost.Show(true);
		m_MoveDD.Toggle();
	}

	//one page of move targets: all of them when they fit the 50 laid-out rows, else MOVE_DD_MAX_ROWS - 1 files plus a
	//last language-neutral '>> from-to / total' row that pages on (wrapping to the first page), so every file stays pickable
	protected void FillMovePage(int page)
	{
		m_MoveFiles.Clear();
		m_MoveLabels.Clear();
		m_MoveDD.RemoveAllElements();
		int total = m_MoveAll.Count();
		int perPage = total;
		if (total > MOVE_DD_MAX_ROWS)
		{
			perPage = MOVE_DD_MAX_ROWS - 1;
		}

		int start = page * perPage;
		if (page < 0 || start >= total)
		{
			page = 0;
			start = 0;
		}

		m_MovePage = page;
		int stopIdx = start + perPage;
		if (stopIdx > total)
		{
			stopIdx = total;
		}

		for (int i = start; i < stopIdx; i++)
		{
			string key = m_MoveAll[i];
			string entry = FitFileKey(key, 21);
			m_MoveFiles.Insert(key);
			m_MoveLabels.Insert(entry);
			m_MoveDD.AddElement(entry);
		}

		if (total <= perPage)
		{
			return;
		}

		int nextStart = stopIdx;
		if (nextStart >= total)
		{
			nextStart = 0;
		}

		int nextEnd = nextStart + perPage;
		if (nextEnd > total)
		{
			nextEnd = total;
		}

		int nextFrom = nextStart + 1;
		string moreRow = ">> " + nextFrom.ToString() + "-" + nextEnd.ToString() + " / " + total.ToString();
		m_MoveDD.AddElement(moreRow);
	}

	//the paging row is handled a frame later: the dropdown closes itself after the select callback, and its clicked row
	//must not be deleted while its OnClick is still running
	protected void ApplyMovePageNext()
	{
		if (m_MovePageNext < 0)
		{
			return;
		}

		int page = m_MovePageNext;
		m_MovePageNext = -1;
		if (!m_MoveDD || !m_MoveTargetHost || !m_MoveTargetHost.IsVisible() || m_PendOp != VPPXEOp.MOVE)
		{
			return;
		}

		FillMovePage(page);
		m_MoveDD.SetText(Tr("#VSTR_XMLE_DLG_MOVE_TITLE"));
		m_MoveDD.SetIndex(-1);
		m_MoveDD.Close();
		m_MoveDD.Toggle();
	}

	void OnMoveTargetSelected(int index)
	{
		if (m_PendOp == VPPXEOp.MOVE && index >= 0 && index == m_MoveFiles.Count() && m_MoveAll.Count() > m_MoveFiles.Count())
		{
			m_MovePageNext = m_MovePage + 1;
			return;
		}

		if (index >= 0 && index < m_MoveLabels.Count())
		{
			m_MoveDD.SetText(m_MoveLabels[index]);
		}

		m_MoveDD.SetIndex(index);
		m_MoveDD.Close();
		CancelMovePick();
		if (index < 0 || index >= m_MoveFiles.Count() || m_PendOp != VPPXEOp.MOVE)
		{
			ClearPending();
			return;
		}

		VPPXEClientModel model = Model();
		m_PendTargetFile = m_MoveFiles[index];
		if (model.DefExists(m_PendTargetFile, m_PendName))
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_ERR_NAME_EXISTS"), m_PendName));
			ClearPending();
			return;
		}

		string effect = model.DescribeEffectChange(m_PendName, VPPXEOp.MOVE, m_PendName, m_PendSrcFile, m_PendTargetFile);
		string body = string.Format(Tr("#VSTR_XMLE_DLG_MOVE_BODY"), m_PendName, model.FileLabel(m_PendTargetFile), effect) + PendingNote(m_PendSrcFile, m_PendName);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_MOVE_TITLE", body, DIAGTYPE.DIAG_YESNOCANCEL, this, "OnMoveChoice", false);
	}

	//YES = move, NO = copy, CANCEL = nothing
	void OnMoveChoice(int result, string input)
	{
		if (result == DIAGRESULT.YES)
		{
			m_PendOp = VPPXEOp.MOVE;
		}
		else if (result == DIAGRESULT.NO)
		{
			m_PendOp = VPPXEOp.COPY;
		}
		else
		{
			ClearPending();
			return;
		}

		VPPXEClientModel model = Model();
		array<string> others = new array<string>();
		if (m_PendOp == VPPXEOp.MOVE)
		{
			model.OtherDefFiles(m_PendName, m_PendTargetFile, m_PendSrcFile, others);
		}
		else
		{
			model.OtherDefFiles(m_PendName, m_PendTargetFile, "", others);
		}

		if (others.Count() > 0 || m_PendOp == VPPXEOp.COPY)
		{
			AskOverride(m_PendName, others, model.DescribeEffectChange(m_PendName, m_PendOp, m_PendName, m_PendSrcFile, m_PendTargetFile));
			return;
		}

		SendStructural(false);
	}

	protected void OnDeleteClicked()
	{
		if (!StructPrecheck())
		{
			return;
		}

		VPPXEClientModel model = Model();
		ClearPending();
		m_PendOp = VPPXEOp.DELETE;
		m_PendName = model.DisplayNameOf(m_TypeName);
		m_PendSrcFile = m_TargetFile;
		string body = string.Format(Tr("#VSTR_XMLE_DLG_DELETE_BODY"), m_PendName, model.FileLabel(m_TargetFile), OtherDefsSentence(m_TypeName, m_TargetFile));
		VPPXEStagedEdit staged = model.GetStaged(m_TargetFile, m_TypeName);
		if (staged)
		{
			body = body + " " + string.Format(Tr("#VSTR_XMLE_DLG_DROP_STAGED"), staged.CountChanges());
		}

		body = body + PendingNote(m_TargetFile, m_TypeName);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_DELETE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmDelete", false);
	}

	void OnConfirmDelete(int result, string input)
	{
		if (result != DIAGRESULT.YES || m_PendOp != VPPXEOp.DELETE)
		{
			ClearPending();
			return;
		}

		SendStructural(false);
	}

	//one batch for the affected file: its staged UPDATEs of OTHER names + the structural edit (Reason STRUCT)
	protected void SendStructural(bool allowOverride)
	{
		VPPXEClientModel model = Model();
		if (m_PendOp < 0 || m_PendSrcFile == "")
		{
			ClearPending();
			return;
		}

		string batchFile = m_PendSrcFile;
		bool twoFiles = m_PendOp == VPPXEOp.COPY || m_PendOp == VPPXEOp.MOVE;
		if (model.IsFileLocked(batchFile) || (twoFiles && model.IsFileLocked(m_PendTargetFile)))
		{
			ClearPending();
			return;
		}

		VPPXETypeEdit edit = new VPPXETypeEdit();
		edit.Op = m_PendOp;
		edit.Name = m_PendName;
		edit.NewName = m_PendNewName;
		edit.AllowOverride = allowOverride;
		if (twoFiles)
		{
			edit.TargetFileKey = m_PendTargetFile;
		}

		if (m_PendOp == VPPXEOp.DELETE)
		{
			model.RevertType(batchFile, m_PendName);
		}

		VPPXEUploadBatch up = model.BuildFileBatch(batchFile, VPPXEBackupReason.STRUCT, edit, m_PendName);
		NotifyOrphans();
		if (!up)
		{
			NotifyTooMany();
			ClearPending();
			return;
		}

		model.LockFile(batchFile, up.ReqId);
		if (twoFiles)
		{
			model.LockFile(m_PendTargetFile, up.ReqId);
		}

		model.RegisterSave(up.ReqId, batchFile);
		string selectName = "";
		if (m_PendOp == VPPXEOp.ADD)
		{
			selectName = m_PendName;
		}
		else if (m_PendOp == VPPXEOp.DUPLICATE || m_PendOp == VPPXEOp.RENAME)
		{
			selectName = m_PendNewName;
		}

		m_SelectAfterSave.Set(up.ReqId, selectName);
		m_Owner.QueueUpload(up);
		m_Owner.SetStatus("#VSTR_XMLE_STATUS_SAVING");
		ClearPending();
		if (m_TypeName != "")
		{
			Render();
		}

		m_Owner.OnStagingChanged("");
	}

	//---------------------------------------------------------------- bulk editor

	void ShowBulk()
	{
		if (!HasPerm(VPPXEPerm.EDIT_TYPES))
		{
			m_Owner.NotifyError("#VSTR_XMLE_ERR_PERMISSION");
			return;
		}

		CancelMovePick();
		HideUnregistered();
		m_BulkMode = true;
		m_InspectorEmpty.Show(false);
		m_InspectorRoot.Show(false);
		m_BulkRoot.Show(true);
		UpdateBulkCount();
		OnResize();
	}

	protected void HideBulk()
	{
		m_BulkMode = false;
		m_BulkRoot.Show(false);
		if (m_TypeName != "")
		{
			ShowInspector();
			Render();
			RenderDetailsTexts();
		}
		else
		{
			ShowEmpty();
		}
	}

	protected void UpdateBulkCount()
	{
		array<string> names = new array<string>();
		m_Owner.GetFilteredNames(names);
		m_TxtBulkCount.SetText(string.Format(Tr("#VSTR_XMLE_BULK_COUNT"), names.Count()));
	}

	protected void ResetBulk()
	{
		for (int i = 0; i < m_BulkModes.Count(); i++)
		{
			m_BulkModes[i] = 0;
			CheckBoxWidget chk = m_BulkChecks[i];
			if (chk)
			{
				chk.SetChecked(false);
			}

			VPPXENumericEditHandler handler = m_BulkHandlers[i];
			if (handler)
			{
				handler.SetEmpty();
			}

			UpdateBulkModeTints(i);
		}

		for (int j = 0; j < m_BulkFlagStates.Count(); j++)
		{
			m_BulkFlagStates[j] = 0;
			UpdateBulkFlagText(j);
		}

		m_BulkCategory.SetAll(VPPXEChipGroup.CHIP_OFF);
		m_BulkCategory.SetState(0, VPPXEChipGroup.CHIP_ON);
		m_BulkUsage.SetAll(VPPXEChipGroup.CHIP_OFF);
		m_BulkValue.SetAll(VPPXEChipGroup.CHIP_OFF);
		m_BulkTag.SetAll(VPPXEChipGroup.CHIP_OFF);
	}

	protected void UpdateBulkModeTints(int idx)
	{
		int active = ARGB(255, 232, 163, 61);
		int inactive = ARGB(255, 154, 160, 166);
		int mode = m_BulkModes[idx];
		TextWidget setTxt = m_BulkSetTxts[idx];
		TextWidget addTxt = m_BulkAddTxts[idx];
		TextWidget scaleTxt = m_BulkScaleTxts[idx];
		if (setTxt)
		{
			if (mode == 0)
			{
				setTxt.SetColor(active);
			}
			else
			{
				setTxt.SetColor(inactive);
			}
		}

		if (addTxt)
		{
			if (mode == 1)
			{
				addTxt.SetColor(active);
			}
			else
			{
				addTxt.SetColor(inactive);
			}
		}

		if (scaleTxt)
		{
			if (mode == 2)
			{
				scaleTxt.SetColor(active);
			}
			else
			{
				scaleTxt.SetColor(inactive);
			}
		}
	}

	protected void UpdateBulkFlagText(int idx)
	{
		TextWidget txt = m_BulkFlagTxts[idx];
		if (!txt)
		{
			return;
		}

		int state = m_BulkFlagStates[idx];
		string stateKey = "#VSTR_XMLE_BULK_KEEP";
		int color = ARGB(255, 154, 160, 166);
		if (state == 1)
		{
			stateKey = "#VSTR_XMLE_BULK_ON";
			color = ARGB(255, 76, 175, 80);
		}
		else if (state == 2)
		{
			stateKey = "#VSTR_XMLE_BULK_OFF";
			color = ARGB(255, 194, 69, 69);
		}

		txt.SetText(Tr(m_FlagLabelKeys[idx]) + ": " + Tr(stateKey));
		txt.SetColor(color);
	}

	protected bool BulkFieldEnabled(int idx)
	{
		CheckBoxWidget chk = m_BulkChecks[idx];
		VPPXENumericEditHandler handler = m_BulkHandlers[idx];
		if (!chk || !handler)
		{
			return false;
		}

		return chk.IsChecked() && handler.HasValue();
	}

	protected bool BulkHasChanges()
	{
		for (int i = 0; i < m_BulkModes.Count(); i++)
		{
			if (BulkFieldEnabled(i))
			{
				return true;
			}
		}

		for (int j = 0; j < m_BulkFlagStates.Count(); j++)
		{
			if (m_BulkFlagStates[j] != 0)
			{
				return true;
			}
		}

		for (int k = 1; k < m_BulkCategory.Count(); k++)
		{
			if (m_BulkCategory.GetState(k) == VPPXEChipGroup.CHIP_ON)
			{
				return true;
			}
		}

		if (ChipGroupHasDelta(m_BulkUsage) || ChipGroupHasDelta(m_BulkValue) || ChipGroupHasDelta(m_BulkTag))
		{
			return true;
		}

		return false;
	}

	protected bool ChipGroupHasDelta(VPPXEChipGroup group)
	{
		for (int i = 0; i < group.Count(); i++)
		{
			int state = group.GetState(i);
			if (state == VPPXEChipGroup.CHIP_ADD || state == VPPXEChipGroup.CHIP_REMOVE)
			{
				return true;
			}
		}

		return false;
	}

	//collects ALL rows of the current filter and search (not only the displayed ones) and asks for confirmation
	protected void ApplyBulk()
	{
		if (!HasPerm(VPPXEPerm.EDIT_TYPES))
		{
			return;
		}

		if (!BulkHasChanges())
		{
			m_Owner.NotifyError("#VSTR_XMLE_BULK_NOTHING");
			return;
		}

		VPPXEClientModel model = Model();
		array<string> names = new array<string>();
		m_Owner.GetFilteredNames(names);
		string scope = m_Owner.GetTypesScope();
		m_BulkNames.Clear();
		m_BulkTargets.Clear();
		map<string, int> perFile = new map<string, int>();
		map<string, int> newPerFile = new map<string, int>();
		array<string> fileOrder = new array<string>();
		int skipped = 0;
		for (int i = 0; i < names.Count(); i++)
		{
			string lower = names[i];
			string target = scope;
			if (target == "")
			{
				target = model.LastDefFile(lower);
			}

			if (target == "" || !model.IsEditable(target) || model.IsFileLocked(target) || !model.DefExists(target, lower))
			{
				skipped++;
				continue;
			}

			m_BulkNames.Insert(lower);
			m_BulkTargets.Insert(target);
			if (!perFile.Contains(target))
			{
				fileOrder.Insert(target);
			}

			perFile.Set(target, perFile.Get(target) + 1);
			if (!model.GetStaged(target, lower))
			{
				newPerFile.Set(target, newPerFile.Get(target) + 1);
			}
		}

		if (m_BulkNames.Count() == 0)
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_BULK_SKIPPED"), skipped));
			return;
		}

		array<string> fileParts = new array<string>();
		for (int j = 0; j < fileOrder.Count(); j++)
		{
			string fileKey = fileOrder[j];
			int total = model.CountStagedInFile(fileKey) + newPerFile.Get(fileKey);
			if (total > VPPXEConst.MAX_EDITS_PER_BATCH)
			{
				m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_BULK_TOO_MANY"), total, model.FileLabel(fileKey), VPPXEConst.MAX_EDITS_PER_BATCH));
				return;
			}

			fileParts.Insert(model.FileLabel(fileKey) + ": " + perFile.Get(fileKey).ToString());
		}

		// arguments in locals first: a script call among the arguments can zero the ones evaluated before it
		int bulkCount = m_BulkNames.Count();
		string filesText = JoinWith(fileParts, "; ");
		string body = string.Format(Tr("#VSTR_XMLE_DLG_BULK_BODY"), bulkCount, filesText);
		if (skipped > 0)
		{
			body = body + " " + string.Format(Tr("#VSTR_XMLE_BULK_SKIPPED"), skipped);
		}

		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_BULK_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmBulk", false);
	}

	void OnConfirmBulk(int result, string input)
	{
		if (result != DIAGRESULT.YES)
		{
			return;
		}

		VPPXEClientModel model = Model();
		VPPXELimits lim = model.GetLimits();
		array<string> touchedFiles = new array<string>();
		for (int i = 0; i < m_BulkNames.Count(); i++)
		{
			string lower = m_BulkNames[i];
			string target = m_BulkTargets[i];
			if (!model.IsEditable(target) || model.IsFileLocked(target) || !model.DefExists(target, lower))
			{
				continue;
			}

			if (touchedFiles.Find(target) < 0)
			{
				touchedFiles.Insert(target);
			}

			ApplyBulkScalars(model, lower, target);
			ApplyBulkFlags(model, lower, target);
			ApplyBulkCategory(model, lim, lower, target);
			ApplyBulkList(model, m_BulkUsage, m_UsagePlain, VPPXEClientModel.LIST_USAGE, VPPXEClientModel.LIST_USAGE_USER, lim.Usages, lim.UsageGroups, lower, target);
			ApplyBulkList(model, m_BulkValue, m_ValuePlain, VPPXEClientModel.LIST_VALUE, VPPXEClientModel.LIST_VALUE_USER, lim.Values, lim.ValueGroups, lower, target);
			ApplyBulkList(model, m_BulkTag, m_BulkTag.Count(), VPPXEClientModel.LIST_TAG, VPPXEClientModel.LIST_TAG, lim.Tags, lim.Tags, lower, target);
		}

		for (int j = 0; j < touchedFiles.Count(); j++)
		{
			if (model.HasPendingEdits(touchedFiles[j]))
			{
				model.MarkBulk(touchedFiles[j]);
			}
		}

		m_BulkNames.Clear();
		m_BulkTargets.Clear();
		HideBulk();
		m_Owner.OnStagingChanged("");
	}

	protected void ApplyBulkScalars(VPPXEClientModel model, string lower, string target)
	{
		for (int i = 0; i < m_FieldBits.Count(); i++)
		{
			if (!BulkFieldEnabled(i))
			{
				continue;
			}

			int bit = m_FieldBits[i];
			int mode = m_BulkModes[i];
			VPPXENumericEditHandler handler = m_BulkHandlers[i];
			int amount = handler.GetValue();
			int baseNum = 0;
			int num;
			string winner;
			if (model.DisplayScalar(lower, target, bit, num, winner))
			{
				baseNum = num;
			}
			else if (model.DisplayScalar(lower, "", bit, num, winner))
			{
				baseNum = num;
			}

			if (IsQuant(bit) && baseNum == -1 && mode != 0)
			{
				continue;
			}

			int result = amount;
			if (mode == 1)
			{
				result = baseNum + amount;
			}
			else if (mode == 2)
			{
				// multiply in float: int*int overflows past int.MAX for large lifetimes
				float scaled = baseNum;
				scaled = scaled * amount / 100.0;
				scaled = Math.Round(scaled);
				if (scaled > FieldMax(bit))
				{
					scaled = FieldMax(bit);
				}

				if (scaled < FieldMin(bit))
				{
					scaled = FieldMin(bit);
				}

				result = scaled;
			}

			model.StageScalar(target, lower, bit, ClampField(bit, result));
		}
	}

	protected void ApplyBulkFlags(VPPXEClientModel model, string lower, string target)
	{
		for (int i = 0; i < m_BulkFlagStates.Count(); i++)
		{
			int state = m_BulkFlagStates[i];
			if (state == 1)
			{
				model.StageFlag(target, lower, VPPXETypeMerge.FlagBitAt(i), true);
			}
			else if (state == 2)
			{
				model.StageFlag(target, lower, VPPXETypeMerge.FlagBitAt(i), false);
			}
		}
	}

	//chip 0 = keep, chip 1 = no category, chip n >= 2 = Categories[n - 2]
	protected void ApplyBulkCategory(VPPXEClientModel model, VPPXELimits lim, string lower, string target)
	{
		for (int i = 1; i < m_BulkCategory.Count(); i++)
		{
			if (m_BulkCategory.GetState(i) != VPPXEChipGroup.CHIP_ON)
			{
				continue;
			}

			if (i == 1)
			{
				model.StageCategory(target, lower, "");
			}
			else if (i - 2 < lim.Categories.Count())
			{
				model.StageCategory(target, lower, lim.Categories[i - 2]);
			}

			return;
		}
	}

	protected void ApplyBulkList(VPPXEClientModel model, VPPXEChipGroup group, int plainCount, int plainKind, int groupKind, array<string> plainNames, array<string> groupNames, string lower, string target)
	{
		for (int i = 0; i < group.Count(); i++)
		{
			int state = group.GetState(i);
			if (state != VPPXEChipGroup.CHIP_ADD && state != VPPXEChipGroup.CHIP_REMOVE)
			{
				continue;
			}

			int kind = ChipListKind(i, plainCount, plainKind, groupKind);
			string itemName = ChipName(i, plainCount, plainNames, groupNames);
			if (itemName == "")
			{
				continue;
			}

			model.StageListName(target, lower, kind, itemName, state == VPPXEChipGroup.CHIP_ADD);
		}
	}

	//---------------------------------------------------------------- preview

	void ReleasePreview()
	{
		if (m_ItemPreview)
		{
			m_ItemPreview.SetItem(null);
			m_ItemPreview.Show(false);
		}

		if (m_PreviewObject && GetGame())
		{
			GetGame().ObjectDelete(m_PreviewObject);
		}

		m_PreviewObject = null;
		m_PreviewName = "";
	}

	//local preview object of the type (off when the class is not in config)
	protected void UpdatePreview()
	{
		string wanted = "";
		if (m_Details && m_Details.ConfigState != VPPXEConfigState.MISSING)
		{
			wanted = m_Details.Name;
		}

		if (wanted == m_PreviewName && (wanted == "" || m_PreviewObject))
		{
			return;
		}

		ReleasePreview();
		m_PreviewName = wanted;
		if (wanted == "" || !m_ItemPreview || GetGame().IsKindOf(wanted, "dz_lightai"))
		{
			return;
		}

		Object created = GetGame().CreateObject(wanted, vector.Zero, true, false, false);
		m_PreviewObject = EntityAI.Cast(created);
		if (!m_PreviewObject)
		{
			if (created)
			{
				GetGame().ObjectDelete(created);
			}

			return;
		}

		m_ItemPreview.SetItem(m_PreviewObject);
		m_ItemPreview.SetModelPosition(Vector(0, 0, 0.5));
		m_ItemPreview.SetModelOrientation(Vector(0, 0, 0));
		m_ItemPreview.SetView(m_PreviewObject.GetViewIndex());
		m_ItemPreview.Show(true);
		m_ItemOrientation = Vector(0, 0, 0);
	}

	void UpdateItemRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		if (!m_ItemPreview)
		{
			return;
		}

		vector orientation = m_ItemOrientation;
		if (m_ItemOrientation[0] == 0 && m_ItemOrientation[1] == 0 && m_ItemOrientation[2] == 0)
		{
			orientation = m_ItemPreview.GetModelOrientation();
			m_ItemOrientation = orientation;
		}

		orientation[0] = orientation[0] + (m_RotationY - mouse_y);
		orientation[1] = orientation[1] - (m_RotationX - mouse_x);
		m_ItemPreview.SetModelOrientation(orientation);
		if (!is_dragging)
		{
			m_ItemOrientation = orientation;
		}
	}

	//---------------------------------------------------------------- events

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (w == m_ItemPreview && m_PreviewObject)
		{
			GetGame().GetDragQueue().Call(this, "UpdateItemRotation");
			GetMousePos(m_RotationX, m_RotationY);
		}

		return false;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!w || button != MouseState.LEFT)
		{
			return false;
		}

		if (w == m_BtnOpenSpawnables)
		{
			if (m_TypeName != "" && m_Owner)
			{
				m_Owner.OpenSpawnablesType(m_TypeName);
			}

			return true;
		}

		if (w == m_BtnUnregRegister)
		{
			BeginRegister();
			return true;
		}

		if (w == m_BtnSave)
		{
			OnSaveClicked();
			return true;
		}

		if (w == m_BtnRevert)
		{
			OnRevertClicked();
			return true;
		}

		if (w == m_BtnRevertAll)
		{
			OnRevertAllClicked();
			return true;
		}

		if (w == m_BtnDuplicate)
		{
			OnDuplicateClicked();
			return true;
		}

		if (w == m_BtnRenameType)
		{
			OnRenameClicked();
			return true;
		}

		if (w == m_BtnMoveType)
		{
			OnMoveClicked();
			return true;
		}

		if (w == m_BtnDeleteType)
		{
			OnDeleteClicked();
			return true;
		}

		if (w == m_BtnBulkClose)
		{
			HideBulk();
			return true;
		}

		if (w == m_BtnBulkApply)
		{
			ApplyBulk();
			return true;
		}

		if (w == m_BtnBulkReset)
		{
			ResetBulk();
			return true;
		}

		for (int i = 0; i < m_FieldBits.Count(); i++)
		{
			if (w == m_BtnDecs[i])
			{
				StepField(i, -1);
				return true;
			}

			if (w == m_BtnIncs[i])
			{
				StepField(i, 1);
				return true;
			}

			if (w == m_BulkSetBtns[i])
			{
				m_BulkModes[i] = 0;
				UpdateBulkModeTints(i);
				return true;
			}

			if (w == m_BulkAddBtns[i])
			{
				m_BulkModes[i] = 1;
				UpdateBulkModeTints(i);
				return true;
			}

			if (w == m_BulkScaleBtns[i])
			{
				m_BulkModes[i] = 2;
				UpdateBulkModeTints(i);
				return true;
			}
		}

		for (int j = 0; j < m_Checks.Count(); j++)
		{
			if (w == m_Checks[j])
			{
				OnFlagClicked(j);
				return true;
			}

			if (w == m_BulkFlagBtns[j])
			{
				m_BulkFlagStates[j] = (m_BulkFlagStates[j] + 1) % 3;
				UpdateBulkFlagText(j);
				return true;
			}
		}

		return false;
	}
};
