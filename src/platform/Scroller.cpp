
/* Copyright (c) 2010, Peter Barrett  
**  
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
**  
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#include "Platform.h"
#include "Scroller.h"

#define GREY(_x) (((_x & 0xF8) << 8) | ((_x & 0xFC) << 3) | ((_x & 0xF8) >> 3))
extern const short _greys[8] PROGMEM; 
const short _greys[8] = 
{
    GREY(0x60),GREY(0x70),GREY(0x80),GREY(0x90),GREY(0xA0),GREY(0xB0),GREY(0xC0),GREY(0xD0)
};  // dark to light

//	Silly just to avoid a divide/mod
int Mod320(int y)
{
	while (y >= 320)
		y -= 320;
	while (y < 0)
		y += 320;	// 
	return y;
}

//  Fill above and below the image
void FillScrollBG(int y, int height, int t)
{
    short end = y + height;
    while (y < end)
    {
        short c = abs(t-y);
        c = min(7,c);
        Graphics.Rectangle(0,Mod320(y++),240,1,pgm_read_word(_greys+c));
    }
}

//  Encapsulate fancy scrolling
void Scroller::Init(long height, ScrollDrawProc drawProc, void* ref, ScrollMode mode)
{
	_drawProc = drawProc;
	_ref = ref;

    _velocity = 0;
    _scrollMode = mode;
    _scroll = 0;
    _scrollHeight = height;
    Graphics.Scroll(0);
	if (height > 0)
		_drawProc(0,0,min(320,height),_ref);
}

void Scroller::Clear(int color)
{
	Graphics.Clear(color);
}

int Scroller::OnEvent(Event* e)
{
	TouchData* t = (TouchData*)e->Data;
	switch (e->Type)
	{
		case Event::TouchDown:
			_dragy = t->y;	// Drag in progress
			break;

		case Event::TouchMove:
			if (_dragy != -1 && _dragy != (short)t->y)
			{
				int delta = -_dragy + t->y;
				if (delta >= -2 && delta <= 2)
					break;
				ScrollBy(delta);        // Also velocity
				_dragy = t->y;
				_velocity = delta << 4; // 12:4 fixed point
			}
			break;

		case Event::TouchUp:
			_dragy = -1;
			break;

		case Event::None:
			AutoScroll();
			break;

		default:;
	}
	return 0;
}

void Scroller::Invalidate(int src, int lines)
{
    // Scroll first
    Graphics.Scroll(-_scroll);
    
    //  Draw whitespace before image
    if (src < 0)
    {
        int c = min(lines,_scroll);
        FillScrollBG(src,c,-1);
        src += c;
        lines -= c;
    }
        
    //  Skip drawing the image for now
    short imgY = src;
    short imgLines = min(_scrollHeight-src,lines);
    if (imgLines > 0)
    {
        src += imgLines;
        lines -= imgLines;
    }
                    
    //  Draw whitespace after image
    if (lines > 0)
        FillScrollBG(src,lines,_scrollHeight);
        
    // Now Draw Image
	while (imgLines > 0)
	{
		int y = Mod320(imgY);
		int height = min(imgLines,320-y);
		_drawProc(imgY,y,height,_ref);
		imgY += height;
		imgLines -= height;
	}
}

//  
void Scroller::ScrollBy(int delta)
{
    if (delta == 0)
        return;
    _scroll += delta;
    if (delta < 0)
        Invalidate(-_scroll+320+delta,-delta);
    else
        Invalidate(-_scroll,delta);
}

// round away from zero
inline short round(short x, short f)
{
    if (x == 0)
        return 0;
    if (x < 0)
		x -= f-1;
	else
		x += f-1;
    return x/f;
}

int Scroller::Acceleration()
{
	int a = 0;  // acceleration
    if (_scrollMode == PageScroll)
    {
        int scrollTarget = ((_scroll - 160)/320)*320;
        scrollTarget = max(-_scrollHeight + 320,scrollTarget);   
        a = scrollTarget - _scroll;
     } else {
		if (_scroll > 0)
			a = -_scroll;
		else {
			int minScroll = -_scrollHeight + 320;
			if (_scroll < minScroll)
				a = minScroll - _scroll;
		}
    }
    return a;
}

bool Scroller::Stopped()
{
	return Acceleration() == 0 && _velocity == 0;
}

//  Scroll towards page boundaries or to visible part of page
void Scroller::AutoScroll()
{
   // if (_velocity == 0)
  //      return;

	if (_dragy != -1)
	{
		_velocity >>= 1;	// bleed off
		return;
	}
    
    //  Apply drag in the opposite direction to velocity
	int a = Acceleration();
    a = round(a-_velocity,4);      
    _velocity += a;
	if (_velocity)
		ScrollBy(round(_velocity,16));    // Also velocity
}
