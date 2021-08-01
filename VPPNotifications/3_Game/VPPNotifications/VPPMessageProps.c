#ifndef VPPNOTIFICATIONS
class VPPMessageProps
{
	string title;
	string message;
	string imagePath;
	float  duration;
	float  fadeInDuration;
	float  imageSizeX;
	float  imageSizeY;

	void VPPMessageProps(string msgTitle, string msg, float time = 5.0, float fadeinTime = 2.0, string imgPath = "", float imgSizeX = 1.0, float imgSizeY = 1.0)
	{
		title     	   = msgTitle;
		message   	   = msg;
		imagePath 	   = imgPath;
		duration  	   = time;
		fadeInDuration = fadeinTime;
		imageSizeX     = imgSizeX;
		imageSizeY     = imgSizeY;
	}
}
#endif