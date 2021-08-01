#ifndef VPPNOTIFICATIONS
class VPPNotificationUI
{
	private const int        m_maxVisible = 5;   //how many cards can show max at same time
	private const float      m_updateTick = 1.0; //in seconds
	private bool 			 m_IsVisible;
	private ref AbstractWave m_SoundWave;
	private float            m_Tick = 0.0;

	private Widget       				   	   m_MenuRoot;
	private GridSpacerWidget 				   m_Grid;
	private ref array<ref VPPNotificationCard> m_NotificationCards;

    void VPPNotificationUI()
	{
		m_NotificationCards = new array<ref VPPNotificationCard>;

		GetDayZGame().Event_OnRPC.Insert( AddToQueue );

		#ifdef VPPNOTIFICATIONS
		m_MenuRoot = GetGame().GetWorkspace().CreateWidgets( "VPPNotifications/GUI/Layouts/VPPNotificationParent.layout" );
		#else
		m_MenuRoot = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/VPPNotificationParent.layout" );
		#endif

		m_Grid     = GridSpacerWidget.Cast(m_MenuRoot.FindAnyWidget("Grid"));
		m_MenuRoot.SetSort(1023,true);

		SetVisible(true);
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(this.OnUpdate);
	}
	
	void ~VPPNotificationUI()
	{
		GetDayZGame().Event_OnRPC.Remove( AddToQueue );
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(this.OnUpdate);
	}

	void AddToQueue(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (!GetGame().IsClient()) return;

		if (rpc_type == RPC_VPP_ADD_NOTIFICATION)
		{
			Param1<ref VPPMessageProps> data;
			if (!ctx.Read(data)) return;
			
			if (CountShowingCards() > m_maxVisible)
			{
				m_NotificationCards.Insert(new VPPNotificationCard(data.param1, m_Grid, false)); //Don't show but add to queue
			}else{
				m_NotificationCards.Insert(new VPPNotificationCard(data.param1, m_Grid)); //Add to queue and show now
				PlaySound();
			}
		}

		if (rpc_type == RPC_VPP_ADD_NOTIFICATIONS)
		{
			Param1<ref array<ref VPPMessageProps>> datas;
			if (!ctx.Read(datas)) return;

			array<ref VPPMessageProps> messages = datas.param1;
			foreach(VPPMessageProps message: messages)
			{
				if (CountShowingCards() > m_maxVisible)
				{
					m_NotificationCards.Insert(new VPPNotificationCard(message, m_Grid, false)); //Don't show but add to queue
				}else{
					m_NotificationCards.Insert(new VPPNotificationCard(message, m_Grid)); //Add to queue and show now
					PlaySound();
				}
			}
		}
	}

	int CountShowingCards()
	{
		int count = 0;
		foreach(VPPNotificationCard card : m_NotificationCards)
		{
			if (card != null && card.IsShowing())
				count++;
		}

		return count;
	}

	void OnUpdate(float tDelta)
	{
		m_Tick += tDelta;
		if (m_Tick >= m_updateTick)
		{
			foreach(ref VPPNotificationCard card : m_NotificationCards)
			{
				if (card != null)
				{
					if (card.IsShowing())
					{
						card.Tick();
					}else if (CountShowingCards() < m_maxVisible){
						card.SetShowing(true);
						PlaySound();
					}
				}
			}
			m_Tick = 0.0;
		}
	}

	void SetVisible(bool state)
	{
		m_IsVisible = state;
		m_MenuRoot.Show(state);
	}

	void PlaySound(string soundSetName = "UISEffect_1_SoundSets", bool loop = false)
	{
		/*Sound Effect
		UISEffect_0_SoundSets //Beep
		UISEffect_1_SoundSets //Click
		UISEffect_2_SoundSets //Hint
		*/

		if ( !m_SoundWave )
		{
			SoundParams soundParams			= new SoundParams( soundSetName );
			SoundObjectBuilder soundBuilder	= new SoundObjectBuilder( soundParams );
			SoundObject soundObject			= soundBuilder.BuildSoundObject();
			soundObject.SetKind( WaveKind.WAVEMUSIC );
			m_SoundWave = GetGame().GetSoundScene().Play2D(soundObject, soundBuilder);
			m_SoundWave.Loop( loop );
			m_SoundWave.Play();
		}
	}
	
	void StopSound()
	{
		if ( m_SoundWave )
		{
			m_SoundWave.Stop();
			delete m_SoundWave;
		}
	}
}
#endif