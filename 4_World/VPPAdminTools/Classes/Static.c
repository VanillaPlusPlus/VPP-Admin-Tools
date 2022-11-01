static void VPPAT_DebugConeDraw(vector start, float cone_angle, float reach_distance)
{
	vector endL, endR;
	float playerAngle;
	float xL,xR,zL,zR;
	
	playerAngle = MiscGameplayFunctions.GetHeadingAngle( PlayerBase.Cast( GetGame().GetPlayer() ) );

	endL = start;
	endR = start;
	xL = reach_distance * Math.Cos( playerAngle + Math.PI_HALF + cone_angle * Math.DEG2RAD ); // x
	zL = reach_distance * Math.Sin( playerAngle + Math.PI_HALF + cone_angle * Math.DEG2RAD ); // z
	xR = reach_distance * Math.Cos( playerAngle + Math.PI_HALF - cone_angle * Math.DEG2RAD ); // x
	zR = reach_distance * Math.Sin( playerAngle + Math.PI_HALF - cone_angle * Math.DEG2RAD ); // z
	endL[0] = endL[0] + xL;
	endL[2] = endL[2] + zL;
	endR[0] = endR[0] + xR;
	endR[2] = endR[2] + zR;

	Debug.DrawLine(start, endL, COLOR_GREEN);
	Debug.DrawLine(start, endR, COLOR_GREEN);
	Debug.DrawLine(endL, endR, COLOR_GREEN);

	//rayShapes.Insert( Debug.DrawLine( start, endL, COLOR_GREEN ) );
	//rayShapes.Insert( Debug.DrawLine( start, endR, COLOR_GREEN ) ) ;
	//rayShapes.Insert( Debug.DrawLine( endL, endR, COLOR_GREEN ) );
}