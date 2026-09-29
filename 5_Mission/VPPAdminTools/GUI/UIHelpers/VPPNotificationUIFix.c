/*
	Fix for vanilla notifications that stay on screen forever after a quick burst.

	Vanilla NotificationUI.RemoveNotification moves an expired notification into m_WidgetTimers (the fade-out map)
	under the key  m_WidgetTimers.Count().ToString() + data.GetTime().ToString().  That key is not unique: with two
	notifications of the same show time fading ("03", "13"), the first one finishing drops the count to 1 and the
	next expiry builds "13" again ("1" + "13" and "11" + "3" collide the same way). The insert then drops or replaces
	a widget, which is no longer in m_Notifications either, so Update never fades or deletes it: it stays forever.

	This override only frees the key vanilla is about to use: a widget already fading under it is moved to a key that
	cannot collide (it keeps fading from where it was). Then super runs unchanged, so the base game and every other
	mod's override behave exactly as before, and NotificationSystem is not touched.
*/
modded class NotificationUI
{
	protected static int s_VPPFadeSerial;

	override void RemoveNotification(NotificationRuntimeData data)
	{
		if (m_WidgetTimers && m_Notifications && m_Notifications.Contains(data))
		{
			int fadingCount = m_WidgetTimers.Count();
			float showTime = data.GetTime();
			string vanillaKey = fadingCount.ToString() + showTime.ToString();
			if (m_WidgetTimers.Contains(vanillaKey))
			{
				Widget fading = m_WidgetTimers.Get(vanillaKey);
				m_WidgetTimers.Remove(vanillaKey);
				string freeKey = VPPFreeFadeKey();
				m_WidgetTimers.Insert(freeKey, fading);
			}
		}

		super.RemoveNotification(data);
	}

	// A fade-out key no vanilla key ("<count><time>") or voice key (a player id) can be.
	protected string VPPFreeFadeKey()
	{
		string key = "VPPFade" + s_VPPFadeSerial.ToString();
		while (m_WidgetTimers.Contains(key))
		{
			s_VPPFadeSerial++;
			key = "VPPFade" + s_VPPFadeSerial.ToString();
		}

		s_VPPFadeSerial++;
		return key;
	}
};
