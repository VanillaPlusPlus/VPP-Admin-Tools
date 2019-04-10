class ItemPreviewInstance
{
	private ItemPreviewWidget m_ItemPreviewWidget;
	private ref array<EntityAI> m_PrewViewObjects;
	private int m_RotationX;
	private int m_RotationY;
	private vector m_ItemOrientation;

	void ItemPreviewInstance( Widget MainRoot )
	{
		m_PrewViewObjects   = new array<EntityAI>;
		m_ItemPreviewWidget = ItemPreviewWidget.Cast( MainRoot );

		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_ItemPreviewWidget,  this, "CallRotation" );
	}

	void ~ItemPreviewInstance()
	{
		for (int i = 0; i < m_PrewViewObjects.Count(); ++i)
	    {
	    	EntityAI obj;
	    	obj = EntityAI.Cast( m_PrewViewObjects.Get(i) );
	    	if (obj != NULL)
	    	{
	    		GetGame().ObjectDelete(obj);
	    	}
	    }
	}

	void SetPrewViewItem(string ClassName)
	{
		if (GetGame().IsKindOf( ClassName, "dz_lightai" )) return;
		bool isSpawned = false;
		EntityAI SpawnedItem;
		if (m_PrewViewObjects != NULL && m_PrewViewObjects.Count() >= 1)
		{
			for (int i = 0; i < m_PrewViewObjects.Count(); ++i)
		    {
		    	EntityAI obj = m_PrewViewObjects.Get(i);
		    	if (obj != NULL)
		    	{
		    		string Type = obj.GetType();
		    		if (Type == ClassName)
		    		{
		    			isSpawned = true;
		    			SpawnedItem = EntityAI.Cast( m_PrewViewObjects.Get(i) );
		    		}
		    	}
		    }
		}
		
	    if (isSpawned && SpawnedItem != NULL)
	    {
	    	m_ItemPreviewWidget.SetItem( SpawnedItem );
			m_ItemPreviewWidget.SetModelPosition( Vector(-5,0,0) );
			m_ItemPreviewWidget.SetModelOrientation( Vector(0,0,0) );
	    }
	    else if (!isSpawned)
	    {
			EntityAI PrewViewItem;
			PrewViewItem = EntityAI.Cast( GetGame().CreateObject( ClassName, "0 0 0", true, false ) );
			if (PrewViewItem != NULL)
			{
			    m_ItemPreviewWidget.SetItem( PrewViewItem );
				m_ItemPreviewWidget.SetModelPosition( Vector(-5,0,0) );
				m_ItemPreviewWidget.SetModelOrientation( Vector(0,0,0) );
				m_PrewViewObjects.Insert(PrewViewItem);
			}
	    }
	}

	void CallRotation()
	{
		GetGame().GetDragQueue().Call(this, "UpdateItemRotation");
		g_Game.GetMousePos(m_RotationX, m_RotationY);
	}

	void UpdateItemRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		vector orientation;
		if (m_ItemOrientation[0] == 0 && m_ItemOrientation[1] == 0 && m_ItemOrientation[2] == 0)
		{
			orientation        = m_ItemPreviewWidget.GetModelOrientation();
			m_ItemOrientation  = m_ItemPreviewWidget.GetModelOrientation();
		}
		else
		{
			orientation = m_ItemOrientation;
		}
		
		orientation[0] = orientation[0] + (m_RotationY - mouse_y);
		orientation[1] = orientation[1] - (m_RotationX - mouse_x);

		m_ItemPreviewWidget.SetModelOrientation( orientation );

		if ( !is_dragging )
		{	
			m_ItemOrientation = orientation;
		}
	}
}