#include "Platform.h"


class NotepadState{
	
	signed char accxyz[3];
	int lastacc;
	
public:
	
	void Draw(int x, int y){
		Graphics.SetPixel(x,y,0);
	}

	int OnEvent(Event* e){
		
		//	test for "shake"
		Hardware.GetAccelerometer(accxyz);
		
		if((accxyz[0] - lastacc) > 25 || (accxyz[0] - lastacc) < -25){
			
			//	clear the screen
			Graphics.Clear(Graphics.ToColor(255,255,255));
		}
		
		lastacc = accxyz[0];
		
		switch (e->Type){
				
			case Event::OpenApp:
				break;
				
			case Event::TouchDown:
			case Event::TouchMove:
				Draw(e->Touch->x, e->Touch->y);
				if(e->Touch->y > 320)
					return -1;
				
			default:
				;
		}
		
		return 0;
	}
};

INSTALL_APP(notepad,NotepadState);