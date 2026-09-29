// XML Editor: distribution service (INTERFACES section 5 VPPXEDistService).
// Owns the LIGHT/MAP/CLUSTERS snapshots, starts the BACKGROUND stage jobs in order LIGHT -> MAP -> CLUSTERS,
// keeps the pending distribution queries (they wait for the types index and the stages, with progress),
// serves FillSpawnable and the cross-file issues, and delegates the live scan and the guarded delete.

class VPPXEDistWaiter : Managed
{
	PlayerIdentity Sender;
	int ReqId;
	int Target;
};

class VPPXEDistPending : Managed
{
	PlayerIdentity Sender;
	string PlainId;
	int ReqId;
	string TypeName;
	string TypeKey;
	int LayerMask;
	int Created;
	int TypesWaitStart;
	bool TypesTriggered;
};

class VPPXEDistService : Managed
{
	const static int TICK_MS = 250;
	const static int REV_CHECK_MS = 2000;
	const static int PENDING_TIMEOUT_MS = 180000;
	const static int CURRENT_WAIT_MS = 10000;
	const static int KEEPALIVE_MS = 5000;
	const static int PROGRESS_RESEND_MS = 1000;
	const static int MAX_SPAWN_LINES = 30;
	const static int MAX_SPAWN_PARENTS = 50;
	const static int MAX_SPAWNABLE_ISSUES = 1000;
	const static string AREAFLAGS_KEY = "areaflags.map";

	protected XMLEditor m_Owner;
	protected ref VPPXESpawnLight m_Light;
	protected ref VPPXESpawnMap m_Map;
	// the types limits object current when the MAP job started (the proto masks use its bit order);
	// the types service replaces that object only when the limits content changes
	protected ref VPPXELimits m_MapLimits;
	protected ref VPPXEClusterData m_Clusters;
	// the mapgroupcluster*.xml keys and revisions taken when the CLUSTERS job started (m_Clusters was read from them)
	protected ref array<string> m_ClusterPosKeys;
	protected ref array<int> m_ClusterPosRevs;
	protected VPPXESpawnLightJob m_LightJob;
	protected VPPXESpawnMapJob m_MapJob;
	protected VPPXEClusterJob m_ClusterJob;
	protected VPPXEAreaFlagsRevalidateJob m_RevalidateJob;
	protected bool m_RevalidateScheduled;
	protected bool m_WantLight;
	protected bool m_WantMap;
	protected bool m_WantClusters;
	protected bool m_Deferred;
	protected bool m_TypesKicked;
	protected bool m_RevCheckNeeded;
	protected int m_LastRevCheck;
	protected ref array<ref VPPXEDistWaiter> m_Waiters;
	protected ref array<ref VPPXEDistPending> m_Pending;
	protected bool m_TickRunning;
	protected bool m_Evaluating;
	protected int m_LastProgStage;
	protected int m_LastProgPct;
	protected int m_LastProgSentMs;
	protected int m_LastKeepAliveMs;
	protected ref map<string, int> m_LastLiveScan;

	void VPPXEDistService(XMLEditor owner)
	{
		m_Owner = owner;
		m_RevalidateScheduled = false;
		m_WantLight = false;
		m_WantMap = false;
		m_WantClusters = false;
		m_Deferred = false;
		m_TypesKicked = false;
		m_RevCheckNeeded = false;
		m_LastRevCheck = 0;
		m_Waiters = new array<ref VPPXEDistWaiter>();
		m_Pending = new array<ref VPPXEDistPending>();
		m_TickRunning = false;
		m_Evaluating = false;
		m_LastProgStage = -1;
		m_LastProgPct = -1;
		m_LastLiveScan = new map<string, int>();
		m_ClusterPosKeys = new array<string>();
		m_ClusterPosRevs = new array<int>();
	}

	void ~VPPXEDistService()
	{
		if (m_TickRunning && GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.Tick);
		}
	}

	// ------------------------------------------------------------------ section 5 API

	bool IsReady(int indexStage)
	{
		CheckRevisions();
		return IsBuilt(indexStage);
	}

	void EnsureStage(int indexStage, PlayerIdentity progressTo, int reqId)
	{
		if (indexStage < VPPXEIndexStage.LIGHT || indexStage > VPPXEIndexStage.CLUSTERS)
		{
			return;
		}

		CheckRevisions();
		if (IsBuilt(indexStage))
		{
			return;
		}

		AddWaiter(progressTo, reqId, indexStage);
		if (!IsBuilt(VPPXEIndexStage.LIGHT))
		{
			m_WantLight = true;
		}

		if (indexStage >= VPPXEIndexStage.MAP && !IsBuilt(VPPXEIndexStage.MAP))
		{
			m_WantMap = true;
		}

		if (indexStage >= VPPXEIndexStage.CLUSTERS && !IsBuilt(VPPXEIndexStage.CLUSTERS))
		{
			m_WantClusters = true;
		}

		Pump();
	}

	void RequestDistribution(PlayerIdentity sender, int reqId, string typeName, int layerMask)
	{
		if (!sender)
		{
			return;
		}

		if (!VPPXmlText.IsValidClassName(typeName))
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_NAME_INVALID", VPPXmlText.Clip(typeName, VPPXEConst.MAX_CELL_CHARS));
			return;
		}

		VPPXELog.Action(sender, "distribution for " + typeName, false);
		string plainId = sender.GetPlainId();
		for (int i = m_Pending.Count() - 1; i >= 0; i--)
		{
			VPPXEDistPending older = m_Pending[i];
			if (older.PlainId == plainId)
			{
				m_Pending.RemoveOrdered(i);
			}
		}

		VPPXEDistPending pending = new VPPXEDistPending();
		pending.Sender = sender;
		pending.PlainId = plainId;
		pending.ReqId = reqId;
		pending.TypeName = typeName;
		pending.TypeKey = VPPXEDistUtil.Lower(typeName);
		pending.LayerMask = layerMask;
		pending.Created = GetGame().GetTime();
		pending.TypesWaitStart = -1;
		pending.TypesTriggered = false;
		m_Pending.Insert(pending);
		EvaluatePending();
	}

	void RequestLiveScan(PlayerIdentity sender, int reqId, string typeName)
	{
		if (!sender)
		{
			return;
		}

		if (!VPPXmlText.IsValidClassName(typeName))
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_NAME_INVALID", VPPXmlText.Clip(typeName, VPPXEConst.MAX_CELL_CHARS));
			return;
		}

		int cooldownMs = 10000;
		int maxResults = 2000;
		XMLEditor editor = Owner();
		if (editor && editor.GetSettings())
		{
			cooldownMs = editor.GetSettings().LiveScanCooldownSec * 1000;
			maxResults = editor.GetSettings().LiveScanMaxResults;
		}

		string plainId = sender.GetPlainId();
		int now = GetGame().GetTime();
		int last;
		if (m_LastLiveScan.Find(plainId, last))
		{
			if (now - last < cooldownMs)
			{
				VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_COOLDOWN", "");
				return;
			}
		}

		m_LastLiveScan.Set(plainId, now);
		VPPXELiveScanJob job = new VPPXELiveScanJob(sender, reqId, typeName, VPPXEDistUtil.WorldSize(), maxResults);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	void DeleteLiveObjects(PlayerIdentity sender, int reqId, string typeName, array<int> netPairs)
	{
		VPPXELiveDelete.Run(sender, reqId, typeName, netPairs);
	}

	// copies the issues precomputed by the LIGHT/MAP stages; SPAWNABLE_NO_TYPE depends on the types index,
	// so it is checked here over the merged spawnable list (one GetEffective lookup per spawnable type).
	// No revision check here: the caller has just checked IsReady, so the reply uses the stages it checked.
	void CollectIssues(array<ref VPPXEIssue> outIssues)
	{
		if (!outIssues)
		{
			return;
		}

		if (m_Light)
		{
			for (int i = 0; i < m_Light.Issues.Count(); i++)
			{
				outIssues.Insert(VPPXEDistUtil.CopyIssue(m_Light.Issues[i]));
			}

			CollectSpawnableNoType(outIssues);
		}

		if (!m_Map)
		{
			return;
		}

		for (int j = 0; j < m_Map.Issues.Count(); j++)
		{
			outIssues.Insert(VPPXEDistUtil.CopyIssue(m_Map.Issues[j]));
		}

		if (m_Map.Samples && !m_Map.Samples.Ok)
		{
			outIssues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.AREAFLAGS, AREAFLAGS_KEY, 0, "", m_Map.Samples.Reason));
		}
	}

	void FillSpawnable(string typeName, VPPXESpawnableInfo info)
	{
		if (!info)
		{
			return;
		}

		if (!m_Light)
		{
			return;
		}

		if (!info.Lines)
		{
			info.Lines = new array<string>();
		}

		if (!info.Parents)
		{
			info.Parents = new array<string>();
		}

		string typeKey = VPPXEDistUtil.Lower(typeName);
		int parentIdx;
		if (m_Light.ParentIndex.Find(typeKey, parentIdx))
		{
			VPPXEDistSpawnParent parent = m_Light.Parents[parentIdx];
			info.Found = true;
			info.Hoarder = parent.Hoarder;
			info.HasDamage = parent.HasDamage;
			info.DamageMin = parent.DamageMin;
			info.DamageMax = parent.DamageMax;
			if (parent.Tag != "")
			{
				info.Lines.Insert(VPPXmlText.Clip("tag " + parent.Tag, VPPXEConst.MAX_CELL_CHARS));
			}

			for (int i = 0; i < parent.Blocks.Count(); i++)
			{
				if (info.Lines.Count() >= MAX_SPAWN_LINES)
				{
					break;
				}

				info.Lines.Insert(BlockLine(parent.Blocks[i]));
			}
		}

		array<int> owners;
		if (!m_Light.ItemParents.Find(typeKey, owners))
		{
			return;
		}

		for (int j = 0; j < owners.Count(); j++)
		{
			if (info.Parents.Count() >= MAX_SPAWN_PARENTS)
			{
				break;
			}

			VPPXEDistSpawnParent owner = m_Light.Parents[owners[j]];
			info.Parents.Insert(owner.Name);
		}
	}

	// the stages are rechecked lazily against the registry revisions they read (next IsReady/EnsureStage/query)
	void OnFilesChanged()
	{
		m_RevCheckNeeded = true;
	}

	// all or nothing: cancel the stage jobs (their destructors close the line streams and the areaflags handle)
	void FreeCaches()
	{
		bool hadLight = m_LightJob != null;
		bool hadMap = m_MapJob != null;
		bool hadClusters = m_ClusterJob != null;
		VPPXEJobQueue queue = VPPXEJobQueue.Get();
		if (m_LightJob)
		{
			queue.Cancel(m_LightJob);
		}

		if (m_MapJob)
		{
			queue.Cancel(m_MapJob);
		}

		if (m_ClusterJob)
		{
			queue.Cancel(m_ClusterJob);
		}

		if (m_RevalidateJob)
		{
			queue.Cancel(m_RevalidateJob);
			m_RevalidateScheduled = false;
		}

		m_LightJob = null;
		m_MapJob = null;
		m_ClusterJob = null;
		m_RevalidateJob = null;
		m_Light = null;
		m_Map = null;
		m_MapLimits = null;
		m_Clusters = null;
		m_WantLight = false;
		m_WantMap = false;
		m_WantClusters = false;
		m_Deferred = false;
		m_TypesKicked = false;
		m_Waiters.Clear();
		FailPending();
		UpdateTick();
		XMLEditor editor = Owner();
		if (!editor)
		{
			return;
		}

		if (hadLight)
		{
			editor.OnIndexStageReady(VPPXEIndexStage.LIGHT, false);
		}

		if (hadLight || hadMap)
		{
			editor.OnIndexStageReady(VPPXEIndexStage.MAP, false);
		}

		if (hadClusters)
		{
			editor.OnIndexStageReady(VPPXEIndexStage.CLUSTERS, false);
		}
	}

	// ------------------------------------------------------------------ stage jobs (called by the jobs)

	static string BuildListSig(VPPXEFileRegistry reg)
	{
		string sig = "";
		sig = AppendKindSig(reg, VPPXEFileKind.RANDOMPRESETS, sig);
		sig = AppendKindSig(reg, VPPXEFileKind.SPAWNABLETYPES, sig);
		sig = AppendKindSig(reg, VPPXEFileKind.EVENTS, sig);
		sig = AppendKindSig(reg, VPPXEFileKind.EVENTGROUPS, sig);
		sig = AppendKindSig(reg, VPPXEFileKind.EVENTSPAWNS, sig);
		sig = AppendKindSig(reg, VPPXEFileKind.CLUSTERPROTO, sig);
		sig = AppendKindSig(reg, VPPXEFileKind.TERRITORY, sig);
		return sig;
	}

	static string AppendKindSig(VPPXEFileRegistry reg, int kind, string sig)
	{
		array<VPPXEFileEntry> list = new array<VPPXEFileEntry>();
		reg.GetByKind(kind, list);
		string result = sig + kind.ToString() + ":";
		for (int i = 0; i < list.Count(); i++)
		{
			VPPXEFileEntry entry = list[i];
			if (!entry)
			{
				continue;
			}

			result = result + entry.Key + "|";
		}

		return result + ";";
	}

	// progress of the running stage to every admin waiting for a stage (VPPXENet throttles further); a step under
	// PROGRESS_STEP_PCT is dropped only while the last send is younger than PROGRESS_RESEND_MS
	void ReportProgress(int stage, int pct)
	{
		int now = GetGame().GetTime();
		if (stage == m_LastProgStage && pct != 0 && pct != 100)
		{
			int delta = pct - m_LastProgPct;
			bool smallStep = delta < VPPXEConst.PROGRESS_STEP_PCT && delta > -VPPXEConst.PROGRESS_STEP_PCT;
			if (smallStep && now - m_LastProgSentMs < PROGRESS_RESEND_MS)
			{
				return;
			}
		}

		m_LastProgStage = stage;
		m_LastProgPct = pct;
		m_LastProgSentMs = now;
		m_LastKeepAliveMs = now;
		for (int i = 0; i < m_Waiters.Count(); i++)
		{
			VPPXEDistWaiter waiter = m_Waiters[i];
			if (waiter.Sender)
			{
				VPPXENet.Progress(waiter.Sender, waiter.ReqId, stage, pct);
			}
		}
	}

	void OnLightJobDone(VPPXESpawnLightJob job, bool ok)
	{
		if (job != m_LightJob)
		{
			return;
		}

		m_LightJob = null;
		if (!ok)
		{
			m_WantLight = false;
			m_WantMap = false;
			m_WantClusters = false;
			FailPending();
			NotifyStage(VPPXEIndexStage.LIGHT, false);
			NotifyStage(VPPXEIndexStage.MAP, false);
			NotifyStage(VPPXEIndexStage.CLUSTERS, false);
			UpdateTick();
			return;
		}

		VPPXESpawnLight data = job.GetLightData();
		if (LightRevsChanged(data))
		{
			VPPXELog.Info("[Dist] LIGHT inputs changed while the stage was built; rebuilding");
			StartLight();
			return;
		}

		m_Light = data;
		m_WantLight = false;
		if (m_Map && (m_Map.SpawnRev != m_Light.SpawnRev || MapLimitsChanged()))
		{
			m_Map = null;
		}

		// the cluster data indexes into the proto it was built from; a new LIGHT brings a new mapclusterproto
		if (m_Clusters && m_Clusters.Proto != m_Light.Clusters)
		{
			m_Clusters = null;
		}

		NotifyStage(VPPXEIndexStage.LIGHT, true);
		Pump();
		CompleteSatisfiedWants();
		EvaluatePending();
	}

	void OnMapJobDone(VPPXESpawnMapJob job, bool ok)
	{
		if (job != m_MapJob)
		{
			return;
		}

		m_MapJob = null;
		if (!ok)
		{
			m_WantMap = false;
			m_WantClusters = false;
			FailPending();
			NotifyStage(VPPXEIndexStage.MAP, false);
			NotifyStage(VPPXEIndexStage.CLUSTERS, false);
			UpdateTick();
			return;
		}

		VPPXESpawnMap data = job.GetMapData();
		if (!m_Light || m_Light.SpawnRev != data.SpawnRev)
		{
			VPPXELog.Info("[Dist] MAP was built on outdated event positions; rebuilding");
			m_WantMap = true;
			Pump();
			return;
		}

		if (MapLimitsChanged())
		{
			VPPXELog.Info("[Dist] MAP was built with outdated types limits; rebuilding");
			m_WantMap = true;
			Pump();
			return;
		}

		m_Map = data;
		m_WantMap = false;
		if (job.IsFromCache())
		{
			ScheduleRevalidation();
		}

		NotifyStage(VPPXEIndexStage.MAP, true);
		Pump();
		CompleteSatisfiedWants();
		EvaluatePending();
	}

	void OnClusterJobDone(VPPXEClusterJob job, bool ok)
	{
		if (job != m_ClusterJob)
		{
			return;
		}

		m_ClusterJob = null;
		if (!ok)
		{
			m_WantClusters = false;
			FailPending();
			NotifyStage(VPPXEIndexStage.CLUSTERS, false);
			UpdateTick();
			return;
		}

		VPPXEClusterData data = job.GetClusterData();
		if (!m_Light || !data || data.Proto != m_Light.Clusters)
		{
			VPPXELog.Info("[Dist] CLUSTERS was built on an outdated cluster proto; rebuilding");
			Pump();
			return;
		}

		if (ClusterPosChanged())
		{
			VPPXELog.Info("[Dist] CLUSTERS inputs changed while the stage was built; rebuilding");
			StartClusters();
			return;
		}

		m_Clusters = data;
		m_WantClusters = false;
		NotifyStage(VPPXEIndexStage.CLUSTERS, true);
		Pump();
		EvaluatePending();
	}

	void OnAreaFlagsRevalidated(VPPXEAreaSamples checkedSamples, VPPXEAreaFlags area)
	{
		m_RevalidateJob = null;
		if (!area)
		{
			return;
		}

		VPPXELog.Info("[Dist] areaflags: " + area.GetResultText());
		if (!area.HasChanged())
		{
			return;
		}

		if (m_Map && m_Map.Samples == checkedSamples)
		{
			m_Map.Samples = area.GetSamples();
		}

		OnFilesChanged();
	}

	// repeating while queries or stage waiters wait, or a stage start is deferred (registry or types index not ready).
	// An admin is waiting, so the job queue keeps its boost slice: the stage jobs run in BACKGROUND and are boosted
	// only through this call.
	void Tick()
	{
		if (m_Pending.Count() > 0 || m_Waiters.Count() > 0)
		{
			VPPXEJobQueue.Get().KeepUrgent(VPPXEJobQueue.URGENT_HOLD_MS);
		}

		if (m_Deferred)
		{
			Pump();
		}

		EvaluatePending();
		KeepWaitersAlive();
		UpdateTick();
	}

	// TYPES progress to every distribution query (they all need the types index); XMLEditor.NotifyTypesProgress
	// throttles the calls.
	void NotifyTypesProgress(int pct)
	{
		for (int i = 0; i < m_Pending.Count(); i++)
		{
			VPPXEDistPending pending = m_Pending[i];
			if (pending && pending.Sender)
			{
				VPPXENet.Progress(pending.Sender, pending.ReqId, VPPXEStage.TYPES, pct);
			}
		}
	}

	// XE_CloseSession: forgets that admin's queries and stage waits. The wanted stages keep building and are cached.
	void DropPendingOf(string plainId)
	{
		for (int i = m_Pending.Count() - 1; i >= 0; i--)
		{
			VPPXEDistPending pending = m_Pending[i];
			if (!pending || pending.PlainId == plainId)
			{
				m_Pending.RemoveOrdered(i);
			}
		}

		for (int j = m_Waiters.Count() - 1; j >= 0; j--)
		{
			VPPXEDistWaiter waiter = m_Waiters[j];
			if (!waiter || !waiter.Sender)
			{
				m_Waiters.RemoveOrdered(j);
				continue;
			}

			if (waiter.Sender.GetPlainId() == plainId)
			{
				m_Waiters.RemoveOrdered(j);
			}
		}

		UpdateTick();
	}

	// ------------------------------------------------------------------ internals

	protected XMLEditor Owner()
	{
		if (m_Owner)
		{
			return m_Owner;
		}

		return GetXMLEditor();
	}

	protected bool IsBuilt(int indexStage)
	{
		if (indexStage == VPPXEIndexStage.LIGHT)
		{
			return m_Light != null;
		}

		if (indexStage == VPPXEIndexStage.MAP)
		{
			return m_Light != null && m_Map != null;
		}

		if (indexStage == VPPXEIndexStage.CLUSTERS)
		{
			return m_Light != null && m_Clusters != null;
		}

		return false;
	}

	// lazy invalidation: after OnFilesChanged, else at most every REV_CHECK_MS (external edits seen by the registry)
	// the limits check is O(1) and runs on every call: a query must never mix item masks in the new bit order
	// with proto masks in the old one
	protected void CheckRevisions()
	{
		if (m_Map && MapLimitsChanged())
		{
			VPPXELog.Info("[Dist] types limits changed; the MAP stage is rebuilt on the next request");
			m_Map = null;
		}

		int now = GetGame().GetTime();
		if (!m_RevCheckNeeded && now - m_LastRevCheck < REV_CHECK_MS)
		{
			return;
		}

		m_RevCheckNeeded = false;
		m_LastRevCheck = now;
		if (m_Light && LightRevsChanged(m_Light))
		{
			VPPXELog.Info("[Dist] LIGHT inputs changed; the stage is rebuilt on the next request");
			m_Light = null;
		}

		if (m_Clusters && ClusterPosChanged())
		{
			VPPXELog.Info("[Dist] CLUSTERS inputs changed; the stage is rebuilt on the next request");
			m_Clusters = null;
		}
	}

	// snapshot of the mapgroupcluster*.xml entries the CLUSTERS job is about to read
	protected void RecordClusterPos()
	{
		m_ClusterPosKeys.Clear();
		m_ClusterPosRevs.Clear();
		XMLEditor editor = Owner();
		if (!editor || !editor.GetRegistry())
		{
			return;
		}

		array<VPPXEFileEntry> list = new array<VPPXEFileEntry>();
		editor.GetRegistry().GetByKind(VPPXEFileKind.CLUSTERPOS, list);
		for (int i = 0; i < list.Count(); i++)
		{
			VPPXEFileEntry entry = list[i];
			if (!entry)
			{
				continue;
			}

			m_ClusterPosKeys.Insert(entry.Key);
			m_ClusterPosRevs.Insert(entry.Revision);
		}
	}

	protected bool ClusterPosChanged()
	{
		XMLEditor editor = Owner();
		if (!editor || !editor.GetRegistry())
		{
			return false;
		}

		VPPXEFileRegistry reg = editor.GetRegistry();
		if (!reg.IsReady())
		{
			return false;
		}

		array<VPPXEFileEntry> list = new array<VPPXEFileEntry>();
		reg.GetByKind(VPPXEFileKind.CLUSTERPOS, list);
		int count = 0;
		for (int i = 0; i < list.Count(); i++)
		{
			VPPXEFileEntry entry = list[i];
			if (!entry)
			{
				continue;
			}

			if (count >= m_ClusterPosKeys.Count())
			{
				return true;
			}

			if (entry.Key != m_ClusterPosKeys[count] || entry.Revision != m_ClusterPosRevs[count])
			{
				return true;
			}

			count++;
		}

		return count != m_ClusterPosKeys.Count();
	}

	protected bool LightRevsChanged(VPPXESpawnLight light)
	{
		XMLEditor editor = Owner();
		if (!editor || !editor.GetRegistry())
		{
			return false;
		}

		VPPXEFileRegistry reg = editor.GetRegistry();
		if (!reg.IsReady())
		{
			return false;
		}

		if (BuildListSig(reg) != light.ListSig)
		{
			return true;
		}

		for (int i = 0; i < light.RecKeys.Count(); i++)
		{
			VPPXEFileEntry entry = reg.Find(light.RecKeys[i]);
			int rev = 0;
			if (entry)
			{
				rev = entry.Revision;
			}

			if (rev != light.RecRevs[i])
			{
				return true;
			}
		}

		return false;
	}

	protected VPPXELimits CurrentLimits()
	{
		XMLEditor editor = Owner();
		if (!editor || !editor.GetTypes())
		{
			return null;
		}

		return editor.GetTypes().GetLimits();
	}

	// the MAP job reads the limits after StartMap, so a limits object replaced in between is also caught here
	protected bool MapLimitsChanged()
	{
		return m_MapLimits != CurrentLimits();
	}

	protected void AddWaiter(PlayerIdentity who, int reqId, int target)
	{
		if (!who)
		{
			return;
		}

		for (int i = 0; i < m_Waiters.Count(); i++)
		{
			VPPXEDistWaiter existing = m_Waiters[i];
			if (existing.Sender == who && existing.ReqId == reqId)
			{
				if (target > existing.Target)
				{
					existing.Target = target;
				}

				return;
			}
		}

		VPPXEDistWaiter waiter = new VPPXEDistWaiter();
		waiter.Sender = who;
		waiter.ReqId = reqId;
		waiter.Target = target;
		m_Waiters.Insert(waiter);
	}

	protected void FinishWaiters(int stage, bool ok)
	{
		for (int i = m_Waiters.Count() - 1; i >= 0; i--)
		{
			VPPXEDistWaiter waiter = m_Waiters[i];
			bool drop = false;
			if (!waiter.Sender)
			{
				drop = true;
			}
			else if (ok && waiter.Target <= stage)
			{
				drop = true;
			}
			else if (!ok && waiter.Target >= stage)
			{
				drop = true;
			}

			if (drop)
			{
				m_Waiters.RemoveOrdered(i);
			}
		}
	}

	// Every KEEPALIVE_MS without a progress send, repeats the last stage and percent to every waiter, so a phase that
	// reports nothing (MAP split, dispatch and points; CLUSTERS within a file; a stage queued behind another) does
	// not expire the client's inactivity timer.
	protected void KeepWaitersAlive()
	{
		if (m_Waiters.Count() == 0 || m_LastProgStage < 0)
		{
			return;
		}

		int now = GetGame().GetTime();
		if (now - m_LastKeepAliveMs < KEEPALIVE_MS)
		{
			return;
		}

		m_LastKeepAliveMs = now;
		for (int i = 0; i < m_Waiters.Count(); i++)
		{
			VPPXEDistWaiter waiter = m_Waiters[i];
			if (waiter && waiter.Sender)
			{
				VPPXENet.Progress(waiter.Sender, waiter.ReqId, m_LastProgStage, m_LastProgPct);
			}
		}
	}

	protected void NotifyStage(int stage, bool ok)
	{
		FinishWaiters(stage, ok);
		XMLEditor editor = Owner();
		if (editor)
		{
			editor.OnIndexStageReady(stage, ok);
		}
	}

	// a wanted stage that is already built after an earlier stage finished (for example MAP kept across a LIGHT rebuild)
	protected void CompleteSatisfiedWants()
	{
		if (m_WantMap && m_Map && m_Light && !m_MapJob)
		{
			m_WantMap = false;
			NotifyStage(VPPXEIndexStage.MAP, true);
		}

		if (m_WantClusters && m_Clusters && m_Light && !m_ClusterJob)
		{
			m_WantClusters = false;
			NotifyStage(VPPXEIndexStage.CLUSTERS, true);
		}
	}

	protected void Pump()
	{
		m_Deferred = false;
		if (m_LightJob || m_MapJob || m_ClusterJob)
		{
			UpdateTick();
			return;
		}

		if (!m_WantLight && !m_WantMap && !m_WantClusters)
		{
			UpdateTick();
			return;
		}

		XMLEditor editor = Owner();
		if (!editor)
		{
			return;
		}

		VPPXEFileRegistry reg = editor.GetRegistry();
		if (!reg || !reg.IsReady())
		{
			m_Deferred = true;
			UpdateTick();
			return;
		}

		if (!m_Light)
		{
			StartLight();
			UpdateTick();
			return;
		}

		if (m_WantMap && !m_Map)
		{
			VPPXETypesService types = editor.GetTypes();
			if (!types || !types.IsReady())
			{
				if (types && !m_TypesKicked)
				{
					m_TypesKicked = true;
					types.EnsureIndex();
				}

				m_Deferred = true;
				UpdateTick();
				return;
			}

			m_TypesKicked = false;
			StartMap();
			UpdateTick();
			return;
		}

		if (m_WantClusters && !m_Clusters)
		{
			StartClusters();
		}

		UpdateTick();
	}

	protected void StartLight()
	{
		VPPXESpawnLightJob job = new VPPXESpawnLightJob(this);
		m_LightJob = job;
		m_LastProgStage = -1;
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.BACKGROUND);
	}

	// MAP needs the types limits (bit order) and the LIGHT event positions
	protected void StartMap()
	{
		m_MapLimits = CurrentLimits();
		VPPXESpawnMapJob job = new VPPXESpawnMapJob(this, m_Light);
		m_MapJob = job;
		m_LastProgStage = -1;
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.BACKGROUND);
	}

	protected void StartClusters()
	{
		RecordClusterPos();
		VPPXEClusterJob job = new VPPXEClusterJob(this, m_Light.Clusters);
		m_ClusterJob = job;
		m_LastProgStage = -1;
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.BACKGROUND);
	}

	// once per boot after a cache hit
	protected void ScheduleRevalidation()
	{
		if (m_RevalidateScheduled || !m_Map || !m_Map.Samples || !m_Map.Samples.Ok)
		{
			return;
		}

		m_RevalidateScheduled = true;
		string worldName = "";
		XMLEditor editor = Owner();
		if (editor && editor.GetRegistry())
		{
			worldName = editor.GetRegistry().GetWorldName();
		}

		VPPXEAreaFlagsRevalidateJob job = new VPPXEAreaFlagsRevalidateJob(this, m_Map.Samples, worldName, m_Map.PosRev, m_Map.SpawnRev);
		m_RevalidateJob = job;
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.BACKGROUND);
	}

	protected void UpdateTick()
	{
		bool need = m_Pending.Count() > 0 || m_Deferred || m_Waiters.Count() > 0;
		if (need && !m_TickRunning)
		{
			m_TickRunning = true;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.Tick, TICK_MS, true);
			return;
		}

		if (!need && m_TickRunning)
		{
			m_TickRunning = false;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.Tick);
		}
	}

	protected void FailPending()
	{
		for (int i = 0; i < m_Pending.Count(); i++)
		{
			VPPXEDistPending pending = m_Pending[i];
			VPPXENet.Result(pending.Sender, pending.ReqId, false, "#VSTR_XMLE_ERR_NOT_READY", "");
		}

		m_Pending.Clear();
	}

	protected void EvaluatePending()
	{
		if (m_Evaluating)
		{
			return;
		}

		m_Evaluating = true;
		CheckRevisions();
		int now = GetGame().GetTime();
		int i = 0;
		while (i < m_Pending.Count())
		{
			VPPXEDistPending pending = m_Pending[i];
			if (!pending.Sender)
			{
				m_Pending.RemoveOrdered(i);
				continue;
			}

			if (now - pending.Created > PENDING_TIMEOUT_MS)
			{
				VPPXENet.Result(pending.Sender, pending.ReqId, false, "#VSTR_XMLE_ERR_NOT_READY", "");
				m_Pending.RemoveOrdered(i);
				continue;
			}

			if (TryStart(pending, now))
			{
				m_Pending.RemoveOrdered(i);
				continue;
			}

			i++;
		}

		m_Evaluating = false;
		UpdateTick();
	}

	// preconditions: types index ready (and current, waited for at most CURRENT_WAIT_MS), LIGHT + MAP, CLUSTERS when needed
	protected bool TryStart(VPPXEDistPending pending, int now)
	{
		XMLEditor editor = Owner();
		if (!editor)
		{
			return false;
		}

		bool typesReady = true;
		VPPXETypesService types = editor.GetTypes();
		if (!types || !types.IsReady())
		{
			if (types && !pending.TypesTriggered)
			{
				pending.TypesTriggered = true;
				types.EnsureIndex();
				VPPXENet.Progress(pending.Sender, pending.ReqId, VPPXEStage.TYPES, 0);
			}

			typesReady = false;
		}
		else if (!types.AllFilesCurrent())
		{
			if (pending.TypesWaitStart < 0)
			{
				pending.TypesWaitStart = now;
				ReindexStale(editor);
				VPPXENet.Progress(pending.Sender, pending.ReqId, VPPXEStage.TYPES, 0);
			}

			if (now - pending.TypesWaitStart < CURRENT_WAIT_MS)
			{
				typesReady = false;
			}
		}

		if (!m_Light || !m_Map)
		{
			EnsureStage(VPPXEIndexStage.MAP, pending.Sender, pending.ReqId);
			return false;
		}

		if (!typesReady)
		{
			return false;
		}

		VPPXEClusterData clusters = null;
		bool wantsClusters = (pending.LayerMask & (1 << VPPXELayer.CLUSTERS)) != 0;
		if (wantsClusters && m_Light.NeedsClusters(pending.TypeKey))
		{
			if (!m_Clusters)
			{
				EnsureStage(VPPXEIndexStage.CLUSTERS, pending.Sender, pending.ReqId);
				return false;
			}

			clusters = m_Clusters;
		}

		VPPXEDistQueryJob job = new VPPXEDistQueryJob(this, pending.Sender, pending.ReqId, pending.TypeName, pending.LayerMask, m_Light, m_Map, clusters);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
		return true;
	}

	protected void ReindexStale(XMLEditor editor)
	{
		VPPXEFileRegistry reg = editor.GetRegistry();
		VPPXETypesService types = editor.GetTypes();
		if (!reg || !types)
		{
			return;
		}

		array<VPPXEFileEntry> list = new array<VPPXEFileEntry>();
		reg.GetByKind(VPPXEFileKind.TYPES, list);
		for (int i = 0; i < list.Count(); i++)
		{
			VPPXEFileEntry entry = list[i];
			if (entry && !types.IsFileCurrent(entry.Key))
			{
				types.ReindexFile(entry.Key);
			}
		}
	}

	protected void CollectSpawnableNoType(array<ref VPPXEIssue> outIssues)
	{
		XMLEditor editor = Owner();
		if (!editor)
		{
			return;
		}

		VPPXETypesService types = editor.GetTypes();
		if (!types || !types.IsReady())
		{
			return;
		}

		int added = 0;
		for (int i = 0; i < m_Light.Parents.Count(); i++)
		{
			VPPXEDistSpawnParent parent = m_Light.Parents[i];
			if (types.GetEffective(parent.Name))
			{
				continue;
			}

			if (added >= MAX_SPAWNABLE_ISSUES)
			{
				break;
			}

			outIssues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.SPAWNABLE_NO_TYPE, parent.FileKey, parent.Line, parent.Name, parent.Name));
			added++;
		}
	}

	// one line of spawnabletypes data per block (file data, not translated)
	protected string BlockLine(VPPXEDistSpawnBlock block)
	{
		string line = block.Kind;
		if (block.IsPreset)
		{
			line = line + " preset " + block.PresetName;
		}

		if (block.ChanceText != "")
		{
			line = line + " chance " + block.ChanceText;
		}

		line = line + ":";
		int count = block.ItemNames.Count();
		for (int i = 0; i < count; i++)
		{
			if (line.Length() > VPPXEConst.MAX_CELL_CHARS)
			{
				break;
			}

			string part = " " + block.ItemNames[i];
			string chance = block.ItemChances[i];
			if (chance != "")
			{
				part = part + " " + chance;
			}

			if (i + 1 < count)
			{
				part = part + ",";
			}

			line = line + part;
		}

		return VPPXmlText.Clip(line, VPPXEConst.MAX_CELL_CHARS);
	}
};
