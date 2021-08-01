class LineElement
{
	string data;
	int    index;
	
	void LineElement(string d, int i)
	{
		data  = d;
		index = i;
	}
};

class XMLParser
{
	/*
		Author: Vanilla++ DayZ SA Project
		Note: This script is only desgined to work with a few specific XMLs (DayZ db loot types xmls)
	
		Example Usage (types.xml only):
		
		XMLParser parser = new XMLParser("$CurrentDir:mpmissions\\dayzOffline.utes\\db\\types.xml");
		if (parser.Load() && parser.VerifyXml())
		{
			array<ref LineElement> data = parser.GetElementString("type","AmmoBox");
			LineElement elementData     = parser.GetElementStringValue("lifetime",data);
			parser.ReplaceElementValue("lifetime",elementData.index, "123456");
			
			parser.SaveChanges();
		}
	*/
	
	private ref array<ref LineElement> m_RawLines; //Hold line + index
	private string 			           FILE_PATH;
	private FileHandle 		           m_FileHandle;
	
	void XMLParser(string filePath)
	{
		FILE_PATH = filePath;
	}
	
	bool Load()
	{
		m_FileHandle = OpenFile(FILE_PATH, FileMode.READ);
		if (m_FileHandle == 0) return false;
		
		m_RawLines = new array<ref LineElement>;
		string line_content = "";
		int char_count = FGets( m_FileHandle,  line_content );
		int line_index = 1;

		while ( char_count != -1 )
		{
			m_RawLines.Insert(new LineElement(line_content,line_index));
			char_count = FGets( m_FileHandle,  line_content );
			line_index++;
		}

		CloseFile(m_FileHandle);
		return true;
	}
	
	bool VerifyXml()
	{
		string sTag = "<?xml";
		string eTag = "?>";
		
		if (m_RawLines == null || m_RawLines.Count() < 0) return false;
		
		foreach(LineElement e : m_RawLines)
		{
			if (e.data.Contains(sTag) && e.data.Contains(eTag))
				return true;
		}
		return false;
	}
	
	array<ref LineElement> GetElementString(string eName, string param = "")
	{
		string sTag, eTag;
		eTag = "</"+eName+">";
		if (param == "")
			sTag = "<"+eName+">";
		else
			sTag = "<"+eName+" name="+"\""+param+"\">";
			
		bool pull = false;
		array<ref LineElement> data = new array<ref LineElement>;
		
		foreach(LineElement e : m_RawLines)
		{
			if (e.data.Contains(sTag))
				pull = true;

			if (e.data.Contains(eTag) && pull)
			{
				data.Insert(new LineElement(e.data,e.index));
				break;
			}
			
			if (pull)
				data.Insert(new LineElement(e.data,e.index));
		}
		return data;
	}
	
	LineElement GetElementStringValue(string eName, array<ref LineElement> externalData)
	{
		LineElement value;
		bool pull = false;
		
		string sTag, eTag;
		eTag = "</"+eName+">";
		sTag = "<"+eName+">";
		
		foreach(LineElement e : externalData)
		{
			string line = e.data;
			int index   = e.index;
			if (line.Contains(sTag))
				pull = true;

			if (line.Contains(eTag) && pull)
			{
				value = new LineElement( CleanString(line, eName), index);
				break;
			}
		}
		return value;
	}
	
	bool ReplaceElementValue(string eName, int line_index, string newValue)
	{
		string rawLine   = m_RawLines.Get(line_index - 1).data;
		string cleanLine = CleanString(rawLine, eName);
		
		if (rawLine != "")
		{
			while(newValue.Contains("\t"))
			{
				newValue.Replace("\t",""); //Remove any extra tabs
			}
			string fn  = "\t\t<"+eName+">" + newValue + "</" + eName + ">";
			m_RawLines.Set(line_index - 1, new LineElement(fn, line_index));
			return true;
		}
		return false;
	}
	
	string CleanString(string input, string elementName)
	{
		string eTag = "</"+elementName+">";
		string sTag = "<"+elementName+">";
		
		input.Replace(eTag,"");
		input.Replace(sTag,"");
		while(input.Contains(" "))
		{
			input.Replace(" ","");
		}
		return input;
	}
	
	void SaveChanges()
	{
		FileHandle saveFile = OpenFile("$profile:VPPAdminTools/Exports/types.xml", FileMode.WRITE);
		if (saveFile == 0) return;
		
		foreach(LineElement e : m_RawLines)
			FPrintln(saveFile, e.data);
		
		CloseFile(saveFile);
	}
};