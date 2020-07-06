#ifndef VPPNOTIFICATIONS
class VPPNotificationCard
{
	private const float 		MAX_ALPHA = 0.7;
	private bool                m_Showing;
	private bool                m_FadeActive;
	private ref Widget 	   		m_Card;
	private TextWidget 	   		m_TitleText;
	private RichTextWidget 		m_ContentText;
	private ImageWidget    		m_InfoImage;
	private ref VPPMessageProps m_NotificationProps;
	ref WidgetFadeTimer	    	m_FadeInTimer;
	ref WidgetFadeTimer	    	m_FadeOutTimer;

	void VPPNotificationCard(VPPMessageProps properties, GridSpacerWidget grid, bool showNow = true)
	{
		#ifndef VPPNOTIFICATIONS
		m_Card 			= GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/VPPNotification.layout", grid);
		#else
		m_Card 			= GetGame().GetWorkspace().CreateWidgets( "VPPNotifications/GUI/Layouts/VPPNotification.layout", grid);
		#endif
		
		m_TitleText 	= TextWidget.Cast(m_Card.FindAnyWidget("TitleText"));
		m_ContentText 	= RichTextWidget.Cast(m_Card.FindAnyWidget("ContentText"));
		m_InfoImage 	= ImageWidget.Cast(m_Card.FindAnyWidget("InfoImage"));

		m_NotificationProps = properties;
		SetTitle(properties.title);
		SetContent(properties.message);
		SetImage(properties.imagePath, properties.imageSizeX, properties.imageSizeY);

		grid.Update();
		SetShowing(showNow);
		m_Card.SetSort(1023,true);

		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(this.OnUpdate);
	}

	void ~VPPNotificationCard()
	{
		if (m_Card != null)
			m_Card.Unlink();

		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(this.OnUpdate);
	}

	void OnUpdate(float tDelta)
	{
		//Fade in limit to alpha...
		if (m_FadeInTimer != null && m_Card.GetAlpha() >= MAX_ALPHA)
			m_FadeInTimer.Stop();
	}

	void SetShowing(bool state)
	{
		m_Showing = state;
		m_Card.Show(state);

		if (m_NotificationProps.fadeInDuration > 0.0 && state)
		{
			//Do FadeIn
			m_FadeInTimer = new WidgetFadeTimer();
			m_FadeInTimer.FadeIn( m_Card, m_NotificationProps.fadeInDuration, true );
		}
	}

	bool IsShowing()
	{
		return m_Showing;
	}

	void SetTitle(string txt)
	{
		m_TitleText.SetText(txt);
		m_TitleText.Update();
	}
	
	void SetContent(string txt)
	{
		m_ContentText.SetText(txt);
		m_ContentText.Update();
	}

	void SetImage(string imgPath, float sizeX = 1.0, float sizeY = 1.0)
	{
		m_InfoImage.LoadImageFile(0, imgPath);
		m_InfoImage.SetSize(sizeX, sizeY, true);
		m_InfoImage.Update();
	}

	VPPMessageProps GetProperties()
	{
		return m_NotificationProps;
	}

	void Tick()
	{
		if (m_NotificationProps == null)
		{
			delete this;
			return;
		}

		if (m_NotificationProps.duration <= 0.0)
		{
			//Check if there is a fade out
			if (m_NotificationProps.fadeInDuration > 0.0 && !m_FadeActive)
			{
				//Do fade out
				m_FadeOutTimer = new WidgetFadeTimer();
				m_FadeOutTimer.FadeOut( m_Card, m_NotificationProps.fadeInDuration, true );
				m_FadeActive = true;
			}
			delete this;
			return;
		}

		m_NotificationProps.duration = m_NotificationProps.duration - 1.0;
	}
};
#endif