// XML Editor: CLUSTERS stage (DIST-ALGO v2.1 R17).
// mapclusterproto (small, DOM, parsed during LIGHT) maps each <cluster name> to its first <de name>.
// The mapgroupcluster* files (about 217k lines on Chernarus) are streamed line by line and aggregated
// into per-DE 100 m cell counts; only aggregated cells are kept. Started only by a query whose type is a
// child of an active event that is the de of some cluster.

class VPPXEClusterProto : Managed
{
	ref array<string> ClusterKeys;
	ref array<string> ClusterNames;
	ref array<int> ClusterDe;
	ref map<string, int> ClusterIndex;
	ref array<string> DeKeys;
	ref array<string> DeNames;
	ref map<string, int> DeIndex;

	void VPPXEClusterProto()
	{
		ClusterKeys = new array<string>();
		ClusterNames = new array<string>();
		ClusterDe = new array<int>();
		ClusterIndex = new map<string, int>();
		DeKeys = new array<string>();
		DeNames = new array<string>();
		DeIndex = new map<string, int>();
	}

	int FindCluster(string clusterKey)
	{
		int idx;
		if (ClusterIndex.Find(clusterKey, idx))
		{
			return idx;
		}

		return -1;
	}

	int FindDe(string deKey)
	{
		int idx;
		if (DeIndex.Find(deKey, idx))
		{
			return idx;
		}

		return -1;
	}

	// <cluster name> with its first <de name>; the first definition of a cluster name wins
	void AddCluster(VPPXmlNode node)
	{
		string clusterName = node.GetAttr("name", "");
		if (clusterName == "")
		{
			return;
		}

		string clusterKey = VPPXEDistUtil.Lower(clusterName);
		if (ClusterIndex.Contains(clusterKey))
		{
			return;
		}

		int deIdx = -1;
		VPPXmlNode deNode = node.FirstChild("de");
		if (deNode)
		{
			string deName = deNode.GetAttr("name", "");
			if (deName != "")
			{
				string deKey = VPPXEDistUtil.Lower(deName);
				if (!DeIndex.Find(deKey, deIdx))
				{
					deIdx = DeKeys.Insert(deKey);
					DeNames.Insert(deName);
					DeIndex.Insert(deKey, deIdx);
				}
			}
		}

		int idx = ClusterKeys.Insert(clusterKey);
		ClusterNames.Insert(clusterName);
		ClusterDe.Insert(deIdx);
		ClusterIndex.Insert(clusterKey, idx);
	}
};

class VPPXEClusterData : Managed
{
	ref VPPXEClusterProto Proto;
	int GridCols;
	int Lines;
	int Files;
	ref array<ref map<int, int>> DeCells;
	ref array<int> DeTotal;
	ref array<int> ClusterCount;
	ref array<float> ClusterX;
	ref array<float> ClusterZ;

	void VPPXEClusterData(VPPXEClusterProto clusterProto, int gridCols)
	{
		Proto = clusterProto;
		GridCols = gridCols;
		Lines = 0;
		Files = 0;
		DeCells = new array<ref map<int, int>>();
		DeTotal = new array<int>();
		ClusterCount = new array<int>();
		ClusterX = new array<float>();
		ClusterZ = new array<float>();
		for (int i = 0; i < clusterProto.DeKeys.Count(); i++)
		{
			DeCells.Insert(new map<int, int>());
			DeTotal.Insert(0);
		}

		for (int j = 0; j < clusterProto.ClusterKeys.Count(); j++)
		{
			ClusterCount.Insert(0);
			ClusterX.Insert(0);
			ClusterZ.Insert(0);
		}
	}

	void AddInstance(int clusterIdx, float x, float z)
	{
		int deIdx = Proto.ClusterDe[clusterIdx];
		if (deIdx < 0)
		{
			return;
		}

		int count = ClusterCount[clusterIdx];
		if (count == 0)
		{
			ClusterX.Set(clusterIdx, x);
			ClusterZ.Set(clusterIdx, z);
		}

		ClusterCount.Set(clusterIdx, count + 1);
		int total = DeTotal[deIdx];
		DeTotal.Set(deIdx, total + 1);
		int cellIdx = VPPXEDistUtil.CellIndex(x, z, GridCols);
		map<int, int> cells = DeCells[deIdx];
		int cellCount;
		if (cells.Find(cellIdx, cellCount))
		{
			cells.Set(cellIdx, cellCount + 1);
			return;
		}

		cells.Insert(cellIdx, 1);
	}
};

class VPPXEClusterJob : VPPXEJob
{
	protected VPPXEDistService m_Service;
	protected ref VPPXEClusterData m_Data;
	protected ref array<string> m_Paths;
	protected ref array<string> m_Keys;
	protected int m_FileIdx;
	protected ref VPPXmlLineStream m_Stream;
	protected bool m_InComment;
	protected bool m_Started;
	protected bool m_Done;
	protected int m_StartMs;

	void VPPXEClusterJob(VPPXEDistService service, VPPXEClusterProto clusterProto)
	{
		m_Service = service;
		m_Data = new VPPXEClusterData(clusterProto, VPPXEDistUtil.GridCols(VPPXEDistUtil.WorldSize()));
		m_Paths = new array<string>();
		m_Keys = new array<string>();
		m_FileIdx = 0;
		m_InComment = false;
		m_Started = false;
		m_Done = false;
		m_StartMs = GetGame().GetTime();
	}

	void ~VPPXEClusterJob()
	{
		if (m_Stream)
		{
			m_Stream.Close();
		}
	}

	VPPXEClusterData GetClusterData()
	{
		return m_Data;
	}

	override string GetLabel()
	{
		return "dist CLUSTERS";
	}

	override bool Step()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Done)
			{
				return true;
			}

			if (!m_Started)
			{
				BeginFiles();
			}
			else if (!m_Stream)
			{
				OpenNextFile();
			}
			else
			{
				LineUnit();
			}
		}

		return m_Done;
	}

	override void OnFinished()
	{
		if (m_Stream)
		{
			m_Stream.Close();
		}

		m_Stream = null;
		if (m_Service)
		{
			m_Service.OnClusterJobDone(this, true);
		}
	}

	override void OnAborted()
	{
		if (m_Stream)
		{
			m_Stream.Close();
		}

		m_Stream = null;
		VPPXELog.Info("[Dist] CLUSTERS stage aborted with a script error");
		if (m_Service)
		{
			m_Service.OnClusterJobDone(this, false);
		}
	}

	// R17 files: mapgroupcluster.xml then mapgroupcluster01..99 that exist (registry load order)
	protected void BeginFiles()
	{
		m_Started = true;
		XMLEditor editor = GetXMLEditor();
		if (!editor || !editor.GetRegistry())
		{
			return;
		}

		array<VPPXEFileEntry> list = new array<VPPXEFileEntry>();
		editor.GetRegistry().GetByKind(VPPXEFileKind.CLUSTERPOS, list);
		for (int i = 0; i < list.Count(); i++)
		{
			VPPXEFileEntry entry = list[i];
			if (!VPPXEDistUtil.Exists(entry))
			{
				continue;
			}

			m_Paths.Insert(entry.Path);
			m_Keys.Insert(entry.Key);
		}

		ReportStageProgress(0);
	}

	protected void OpenNextFile()
	{
		if (m_FileIdx >= m_Paths.Count())
		{
			FinishStage();
			return;
		}

		m_Stream = new VPPXmlLineStream();
		m_InComment = false;
		if (!m_Stream.Open(m_Paths[m_FileIdx]))
		{
			VPPXELog.Info("[Dist] CLUSTERS could not open " + m_Keys[m_FileIdx]);
			m_Stream = null;
			m_FileIdx++;
		}
	}

	protected void LineUnit()
	{
		string line;
		if (!m_Stream.Next(line))
		{
			m_Data.Lines = m_Data.Lines + m_Stream.LineNo();
			m_Data.Files = m_Data.Files + 1;
			m_Stream.Close();
			m_Stream = null;
			m_FileIdx++;
			int fileCount = m_Paths.Count();
			if (fileCount < 1)
			{
				fileCount = 1;
			}

			ReportStageProgress(m_FileIdx * 100 / fileCount);
			return;
		}

		ProcessLine(line);

		// Progress inside a file every 1024 lines (a mapgroupcluster file has about 50k lines); int math only.
		int lineNo = m_Stream.LineNo();
		int lineRem = lineNo % 1024;
		if (lineRem != 0)
		{
			return;
		}

		int files = m_Paths.Count();
		if (files < 1)
		{
			files = 1;
		}

		int within = lineNo * 100 / (lineNo + 20000);
		int pct = (m_FileIdx * 100 + within) / files;
		ReportStageProgress(pct);
	}

	// <group name pos> lines; a line inside a comment is skipped
	protected void ProcessLine(string line)
	{
		if (m_InComment)
		{
			if (line.IndexOf("-->") >= 0)
			{
				m_InComment = false;
			}

			return;
		}

		int commentStart = line.IndexOf("<!--");
		if (commentStart >= 0)
		{
			if (line.IndexOfFrom(commentStart + 4, "-->") < 0)
			{
				m_InComment = true;
			}

			return;
		}

		if (line.IndexOf("<group") < 0)
		{
			return;
		}

		string clusterName;
		if (!VPPXmlLineScanner.Attr(line, "name", clusterName))
		{
			return;
		}

		int clusterIdx = m_Data.Proto.FindCluster(VPPXEDistUtil.Lower(clusterName));
		if (clusterIdx < 0)
		{
			return;
		}

		string posValue;
		if (!VPPXmlLineScanner.Attr(line, "pos", posValue))
		{
			return;
		}

		float x;
		float z;
		if (!VPPXmlLineScanner.XZ(posValue, x, z))
		{
			return;
		}

		m_Data.AddInstance(clusterIdx, x, z);
	}

	protected void FinishStage()
	{
		m_Done = true;
		int cellTotal = 0;
		for (int i = 0; i < m_Data.DeCells.Count(); i++)
		{
			map<int, int> cells = m_Data.DeCells[i];
			cellTotal = cellTotal + cells.Count();
		}

		int ms = GetGame().GetTime() - m_StartMs;
		string text = "[Dist] CLUSTERS built in " + ms.ToString() + " ms: " + m_Data.Lines.ToString() + " lines from " + m_Data.Files.ToString() + " files, ";
		text = text + m_Data.Proto.DeKeys.Count().ToString() + " DEs, " + cellTotal.ToString() + " DE cells";
		VPPXELog.Info(text);
		ReportStageProgress(100);
	}

	protected void ReportStageProgress(int pct)
	{
		if (m_Service)
		{
			m_Service.ReportProgress(VPPXEStage.CLUSTERS, pct);
		}
	}
};
