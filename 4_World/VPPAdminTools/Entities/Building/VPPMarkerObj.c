class VPPMarkerObj: House
{
	void VPPMarkerObj()
	{
	}

	void ~VPPMarkerObj()
	{
	}

	void TransformUpdate()
	{
		float radius = 2.0;
		set<Object> hit = new set<Object>;
		vector pos = RaycastNearMouse(hit);
		vector mat[4] = {Vector(radius / 10, 0, 0), Vector(0, radius / 10, 0), Vector(0, 0, radius / 10), Vector(pos[0], pos[1]/*GetGame().SurfaceY(pos[0], pos[2])*/, pos[2])};
		SetTransform(mat);
		PlaceOnSurface();
	}

	private vector RaycastNearMouse(out set<Object> hit, Object ignore = null, float raycast_distance = 1000, float radius = 0, bool groundonly = false)
	{
		vector ray_start = GetGame().GetCurrentCameraPosition();
		vector ray_end = ray_start + GetGame().GetPointerDirection() * raycast_distance;
		
		vector hitPos, hitNormal;
		int hitComponentIndex;		
		hit = new set<Object>;
		
		Object hit_object;
		float hit_else;
		if (!groundonly && DayZPhysics.RayCastBullet(ray_start, ray_end, PhxInteractionLayers.BUILDING, ignore, hit_object, hitPos, hitNormal, hit_else))
			return hitPos;

		DayZPhysics.RaycastRV(ray_start, ray_end, hitPos, hitNormal, hitComponentIndex, hit, null, ignore, false, groundonly, 1, radius, CollisionFlags.ALLOBJECTS);
		return hitPos;
	}
};