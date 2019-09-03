class DEXMLUI
{
	private ref MenuServerManager m_RootClass;

	private ItemPreviewWidget m_ItemPreviewXML;
	private EditBoxWidget	  m_SearchBoxXML;
	private TextListboxWidget m_ItemListBoxXML;
	private ButtonWidget      m_BtnLoadSelected;
	private ButtonWidget 	  m_XMLEditorBTN;
	private Widget       	  m_Panel_XMLEditor;

	protected EditBoxWidget	  m_InputNominal;
	protected EditBoxWidget	  m_InputLifetime;
	protected EditBoxWidget	  m_InputRestock;
	protected EditBoxWidget	  m_InputMin;
	protected EditBoxWidget	  m_InputQuantmin;
	protected EditBoxWidget	  m_InputQuantmax;
	protected EditBoxWidget	  m_InputCost;
	private TextWidget        m_TxtXMLStatus;

	private string m_LastSeleted;
	private ref array<string> m_FieldTypes = { "nominal", "lifetime", "restock", "min", "quantmin", "quantmax", "cost"};
	protected int m_nominalKey,m_lifetimeKey,m_restockKey,m_minKey,m_quantminKey,m_quantmaxKey,m_costKey;

	void DEXMLUI(MenuServerManager RootClass, Widget RootWidget)
	{
		m_RootClass = RootClass;
		GetRPCManager().AddRPC( "RPC_DEXML_UI", "SortData", this, SingeplayerExecutionType.Server );

		m_InputNominal    = EditBoxWidget.Cast( RootWidget.FindAnyWidget( "InputNominal" ) );
		m_InputLifetime   = EditBoxWidget.Cast( RootWidget.FindAnyWidget( "InputLifetime" ) );
		m_InputRestock    = EditBoxWidget.Cast( RootWidget.FindAnyWidget( "InputRestock" ) );
		m_InputMin        = EditBoxWidget.Cast( RootWidget.FindAnyWidget( "InputMin" ) );
		m_InputQuantmin   = EditBoxWidget.Cast( RootWidget.FindAnyWidget( "InputQuantmin" ) );
		m_InputQuantmax   = EditBoxWidget.Cast( RootWidget.FindAnyWidget( "InputQuantmax" ) );
		m_InputCost       = EditBoxWidget.Cast( RootWidget.FindAnyWidget( "InputCost" ) );
		m_TxtXMLStatus    = TextWidget.Cast( RootWidget.FindAnyWidget( "TxtXMLStatus" ) );
	}

	void ~DEXMLUI()
	{
		
	}

	map<int, ref Param2<string,string>> GetNewValues()
	{
		ref map<int, ref Param2<string,string>> m_Data = new map<int, ref Param2<string,string>>;
		ref Param2<string,string> NewValues;

		NewValues = new Param2<string,string>("nominal",m_InputNominal.GetText());
		m_Data.Insert(m_nominalKey,NewValues);

		NewValues = new Param2<string,string>("lifetime",m_InputLifetime.GetText());
		m_Data.Insert(m_lifetimeKey,NewValues);

		NewValues = new Param2<string,string>("restock",m_InputRestock.GetText());
		m_Data.Insert(m_restockKey,NewValues);

		NewValues = new Param2<string,string>("min",m_InputMin.GetText());
		m_Data.Insert(m_minKey,NewValues);

		NewValues = new Param2<string,string>("quantmin",m_InputQuantmin.GetText());
		m_Data.Insert(m_quantminKey,NewValues);

		NewValues = new Param2<string,string>("quantmax",m_InputQuantmax.GetText());
		m_Data.Insert(m_quantmaxKey,NewValues);

		NewValues = new Param2<string,string>("cost",m_InputCost.GetText());
		m_Data.Insert(m_costKey,NewValues);

		return m_Data;
	}

	void SetLastSelected(string text)
	{
		m_LastSeleted = text;
	}

	string GetLastSelected()
	{
		return m_LastSeleted;
	}

	void EmptyInputs(string text)
	{
		if (text == "error") 
		{
			m_InputNominal.SetText(text);
			m_InputLifetime.SetText(text);
			m_InputRestock.SetText(text);
			m_InputMin.SetText(text);
			m_InputQuantmin.SetText(text);
			m_InputQuantmax.SetText(text);
			m_InputCost.SetText(text);
		}
		else
		{
			m_InputNominal.SetText("0");
			m_InputLifetime.SetText("0");
			m_InputRestock.SetText("0");
			m_InputMin.SetText("0");
			m_InputQuantmin.SetText("0");
			m_InputQuantmax.SetText("0");
			m_InputCost.SetText("0");
		}
	}

	void SetStatusText(string text)
	{
		m_TxtXMLStatus.SetText(text);
	}

	void SortData( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param3<int,string,string> data;
		//StartPos,Result,fieldname
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Client)
        {
        	Print("StartPos: "+data.param1.ToString() + " SearchStr: "+data.param2 + " SectionName: "+data.param3);
        	//int = line Position, string field name, string new value
        	switch(data.param3)
        	{
        		case "nominal":
        		m_InputNominal.SetText(data.param2.Trim());
        		m_nominalKey = data.param1;
        		SetStatusText("Successfully Recevied Information from Server!");
        		break;

        		case "lifetime":
        		m_InputLifetime.SetText(data.param2.Trim());
        		m_lifetimeKey = data.param1;
        		SetStatusText("Successfully Recevied Information from Server!");
        		break;

        		case "restock":
        		m_InputRestock.SetText(data.param2.Trim());
        		m_restockKey = data.param1;
        		SetStatusText("Successfully Recevied Information from Server!");
        		break;

        		case "min":
        		m_InputMin.SetText(data.param2.Trim());
        		m_minKey = data.param1;
        		SetStatusText("Successfully Recevied Information from Server!");
        		break;

        		case "quantmin":
        		m_InputQuantmin.SetText(data.param2.Trim());
        		m_quantminKey = data.param1;
        		SetStatusText("Successfully Recevied Information from Server!");
        		break;

        		case "quantmax":
        		m_InputQuantmax.SetText(data.param2.Trim());
        		m_quantmaxKey = data.param1;
        		SetStatusText("Successfully Recevied Information from Server!");
        		break;

        		case "cost":
        		m_InputCost.SetText(data.param2.Trim());
        		m_costKey = data.param1;
        		SetStatusText("Successfully Recevied Information from Server!");
        		break;

        		case "error":
        		SetStatusText("ERROR! Selected Item does not exist in Types.xml!");
        		EmptyInputs("error");
        		break;
        	}
        }
	}

	void GetXMLData(string ClassName)
	{
		foreach(string type : m_FieldTypes)
        {
			Param2<string,string> map_param = new Param2<string,string>(ClassName,type);
        	GetRPCManager().SendRPC( "RPC_ReadFromXML", "GetTypesXML", map_param );
		}
	}
}