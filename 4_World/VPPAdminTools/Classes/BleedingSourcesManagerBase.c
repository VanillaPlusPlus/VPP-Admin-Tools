modded class BleedingSourcesManagerBase
{
	bool RemoveBleedingSourceNonInfectious(int bit)
	{
		if (m_BleedingSources.Contains(bit))
		{
			m_BleedingSources.Remove(bit);
			m_Player.OnBleedingSourceRemoved();

			int inverse_bit_mask = ~bit;
			m_Player.SetBleedingBits(m_Player.GetBleedingBits() & inverse_bit_mask );
			return true;
		}
		return false;
	}
};