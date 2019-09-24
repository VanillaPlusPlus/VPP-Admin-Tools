class XMLParser
{
	/*
		Author: Vanilla++ DayZ SA Project
		Note: This script is only desgin to work with a few specific XMLs (DayZ db loot types xmls) 
	*/
	
	private ref array<string> m_RawLines;
	private string 			  FILE_PATH;
	private FileHandle 		  m_FileHandle;
	
	void XMLParser(string filePath)
	{
		FILE_PATH = filePath;
		Print("Init XMLParser...file: "+filePath);
	}
	
	bool Load()
	{
		m_FileHandle = OpenFile(FILE_PATH, FileMode.READ);
		if (m_FileHandle == 0) return false;
		
		m_RawLines = new array<string>;
		string line_content = "";
		int char_count = FGets( m_FileHandle,  line_content );
		while ( char_count != -1 )
		{
			m_RawLines.Insert(line_content);
			char_count = FGets( m_FileHandle,  line_content );
		}

		CloseFile(m_FileHandle);
		return true;
	}
	
	bool VerifyXml()
	{
		string sTag = "<?xml";
		string eTag = "?>";
		
		if (m_RawLines == null || m_RawLines.Count() < 0) return false;
		
		foreach(string line : m_RawLines)
		{
			if (line.Contains(sTag) && line.Contains(eTag))
			{
				Print("Valid Xml file :D");
				return true;
			}
		}
		Print("Invalid Xml file :(");
		return false;
	}
	
	array<string> GetElementString(string eName, string param = "")
	{
		string sTag, eTag;
		eTag = "</"+eName+">";
		if (param == "")
			sTag = "<"+eName+">";
		else
			sTag = "<"+eName+" name="+"\""+param+"\">";
			
		bool   pull = false;
		autoptr array<string> data = new array<string>;
		
		foreach(string line : m_RawLines)
		{
			if (line.Contains(sTag))
			{
				//Hit Start line of tag
				Print("Hit-Start of Element: "+sTag+" "+eTag);
				pull = true;
			}

			if (line.Contains(eTag) && pull)
			{
				data.Insert(line);
				Print("Hit-End of Element: "+sTag+" "+eTag);
				break;
			}
			
			if (pull)
				data.Insert(line);
		}
		return data;
	}
};