/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsMacMessagePump.h"
#include "nsWindow.h"


#define IsUserWindow(wp) (wp && ((((WindowPeek)wp)->windowKind) >= userKind))

nsWindow* nsMacMessagePump::gCurrentWindow = nsnull;   
nsWindow* nsMacMessagePump::gGrabWindow = nsnull;			// need this for grabmouse


//==============================================================

nsMacMessagePump::nsMacMessagePump(nsMacMessenger *aTheMessageProc)
{

	mMessenger = aTheMessageProc;
	mRunning = PR_FALSE;

}


//==============================================================

nsMacMessagePump::~nsMacMessagePump()
{

}

//==============================================================

PRBool 
nsMacMessagePump::DoMessagePump()
{
PRBool				stillrunning = PR_TRUE;
EventRecord		theevent;
long					sleep=0;
PRInt16				haveevent;
WindowPtr			whichwindow;
RgnHandle			currgn;

#define SUSPENDRESUMEMESSAGE 		0x01
#define MOUSEMOVEDMESSAGE				0xFA

	mRunning = PR_TRUE;
	mInBackground = PR_FALSE;
	
	// calculate the regions to watch
	currgn = ::NewRgn();
	SetRectRgn(currgn,-32000,-32000,-32000,-32000);
	
	
	while(mRunning && stillrunning)
		{			
		haveevent = ::WaitNextEvent(everyEvent,&theevent,sleep,currgn);

		if(haveevent)
			{
			switch(theevent.what)
				{
				case diskEvt:
					if(theevent.message<0)
						{
						// error, bad disk mount
						}
					break;
				case keyUp:
					break;
				case keyDown:
				case autoKey:
					DoKey(&theevent);
					break;
				case mouseDown:
					DoMouseDown(&theevent);
					break;
				case mouseUp:
					DoMouseUp(&theevent);
					break;
				case updateEvt:
					DoPaintEvent(&theevent);
					break;
				case activateEvt:
					whichwindow = (WindowPtr)theevent.message;
					SetPort(whichwindow);
					if(theevent.modifiers & activeFlag)
						{
						::BringToFront(whichwindow);
						::HiliteWindow(whichwindow,TRUE);
						}
					else
						{
						::HiliteWindow(whichwindow,FALSE);
						}
					break;
				case osEvt:
					unsigned char	evtype;
					
					whichwindow = (WindowPtr)theevent.message;
					evtype = (unsigned char) (theevent.message>>24)&0x00ff;
					switch(evtype)
						{
						case MOUSEMOVEDMESSAGE:
							DoMouseMove(&theevent);
							break;
						case SUSPENDRESUMEMESSAGE:
							if(theevent.message&0x00000001)
								{
								// resume message
								mInBackground = PR_FALSE;
								}
							else
								{
								// suspend message
								mInBackground = PR_TRUE;
								}
						}
					break;
				}
			}
		else
			{
			switch(theevent.what)
				{
				case nullEvent:
					DoIdleWidgets();	
					break;
				}		
			}
			
		if(mMessenger)
			stillrunning = mMessenger->IsRunning();
		}

    //if (mDispatchListener)
      //mDispatchListener->AfterDispatch();

  return NS_OK;
}

//==============================================================

void 
nsMacMessagePump::DoPaintEvent(EventRecord *aTheEvent)
{
GrafPtr				curport;
WindowPtr			whichwindow;
nsWindow			*thewindow;
nsRect 				rect;
RgnHandle			updateregion;
 
 	::GetPort(&curport);
	whichwindow = (WindowPtr)aTheEvent->message;
	
	if (IsUserWindow(whichwindow)) 
		{
		SetPort(whichwindow);
		BeginUpdate(whichwindow);
		
		thewindow = (nsWindow*)(((WindowPeek)whichwindow)->refCon);
		if(thewindow != nsnull)
			{
			updateregion = whichwindow->visRgn;
			thewindow->DoPaintWidgets(updateregion);
	    }
		EndUpdate(whichwindow);
		}
	
	::SetPort(curport);

}

//==============================================================

void 
nsMacMessagePump::DoIdleWidgets()
{
WindowPtr			whichwindow;
nsWindow			*thewindow;
		
	whichwindow = ::FrontWindow();
	while(whichwindow)
		{
		// idle the widget
		thewindow = (nsWindow*)(((WindowPeek)whichwindow)->refCon);
		
		whichwindow = (WindowPtr)((WindowPeek)whichwindow)->nextWindow;
		}

}

//==============================================================

void 
nsMacMessagePump::DoMouseDown(EventRecord *aTheEvent)
{
Rect					therect;
Point					windowcoord;
long					newsize;			// window's new size
WindowPtr			whichwindow;
PRInt16				partcode;
nsWindow			*thewindow;
nsMouseEvent	mouseevent;

	partcode = FindWindow(aTheEvent->where,&whichwindow);

	if(whichwindow!=0)
		{
		SelectWindow(whichwindow);
		thewindow = (nsWindow*)(((WindowPeek)whichwindow)->refCon);
			
		if(thewindow != nsnull)
			{
			thewindow = thewindow->FindWidgetHit(aTheEvent->where);
			}

		switch(partcode)
			{
			case inSysWindow:
				break;
			case inContent:
				if(thewindow)
					{
					SetPort(whichwindow);					
					mouseevent.message = NS_MOUSE_LEFT_BUTTON_DOWN;
					mouseevent.widget  = (nsWindow *) thewindow;
					windowcoord = aTheEvent->where;
					GlobalToLocal(&windowcoord);
					mouseevent.point.x = windowcoord.h;
					mouseevent.point.y = windowcoord.v;					
					mouseevent.time = 0;
					mouseevent.isShift = FALSE;
					mouseevent.isControl = FALSE;
					mouseevent.isAlt = FALSE;
					mouseevent.clickCount = 1;
					mouseevent.eventStructType = NS_MOUSE_EVENT;
					thewindow->DispatchMouseEvent(mouseevent);
					gGrabWindow = (nsWindow*)thewindow;		// grab is in effect
					this->SetCurrentWindow(thewindow);
					}
				break;
			case inDrag:
				therect = qd.screenBits.bounds;
				InsetRect(&therect, 3,3);
				therect.top += 20;    /* Allow space for menu bar */
				DragWindow(whichwindow, aTheEvent->where, &therect);
				therect = whichwindow->portRect;
				if (thewindow != nsnull)
				{
					LocalToGlobal(&topLeft(therect));
					LocalToGlobal(&botRight(therect));
					thewindow->SetBounds(therect);
				}
				break;
				
			case inGrow:
				SetPort(whichwindow);
				therect = whichwindow->portRect;
				EraseRect(&therect); 
				InvalRect(&therect);
				
				// Set up the window's allowed minimum and maximum sizes
				therect.bottom = qd.screenBits.bounds.bottom;
				therect.right = qd.screenBits.bounds.right;
				therect.top = therect.left = 75;
				newsize = GrowWindow(whichwindow, aTheEvent->where, &therect);
				if(newsize != 0)
					SizeWindow(whichwindow, newsize & 0x0FFFF, (newsize >> 16) & 0x0FFFF, true);

				// Draw the grow icon & validate that area
				therect = whichwindow->portRect;
				therect.left = therect.right - 16;
				therect.top = therect.bottom - 16;
				DrawGrowIcon(whichwindow);
				ValidRect(&therect);
				
				if (thewindow != nsnull)
				{
					therect = whichwindow->portRect;
					LocalToGlobal(&topLeft(therect));
					LocalToGlobal(&botRight(therect));
					thewindow->SetBounds(therect);
				}

				break;
			case inGoAway:
				if(TrackGoAway(whichwindow,aTheEvent->where))
					if(thewindow)
						{
						thewindow->Destroy();
						mRunning = PR_FALSE;
						}
					else
						{
						}
				break;
			case inZoomIn:
			case inZoomOut:
				break;
			case inMenuBar:
				break;
			}
		}
}

//==============================================================

void 
nsMacMessagePump::DoMouseUp(EventRecord *aTheEvent)
{
WindowPtr			whichwindow;
PRInt16				partcode;
Point					windowcoord;
nsWindow			*thewindow;
nsMouseEvent	mouseevent;

	partcode = FindWindow(aTheEvent->where,&whichwindow);

	if(gGrabWindow)
		{
		mouseevent.message = NS_MOUSE_LEFT_BUTTON_UP;
		mouseevent.widget  = (nsWindow *) gGrabWindow;
		windowcoord = aTheEvent->where;
		GlobalToLocal(&windowcoord);
		mouseevent.point.x = windowcoord.h;
		mouseevent.point.y = windowcoord.v;					
		mouseevent.time = 0;
		mouseevent.isShift = FALSE;
		mouseevent.isControl = FALSE;
		mouseevent.isAlt = FALSE;
		mouseevent.clickCount = 1;
		mouseevent.eventStructType = NS_MOUSE_EVENT;
		gGrabWindow->DispatchMouseEvent(mouseevent);
		gGrabWindow = nsnull;		// mouse grab no longer in effect
		return;
		}

	if(whichwindow!=0)
		{
		SelectWindow(whichwindow);
		thewindow = (nsWindow*)(((WindowPeek)whichwindow)->refCon);
			
		if(thewindow != nsnull)
			thewindow = thewindow->FindWidgetHit(aTheEvent->where);

		switch(partcode)
			{
			case inSysWindow:
				break;
			case inContent:
				if(thewindow)
					{
					SetPort(whichwindow);
					mouseevent.message = NS_MOUSE_LEFT_BUTTON_UP;
					mouseevent.widget  = (nsWindow *) thewindow;
					windowcoord = aTheEvent->where;
					GlobalToLocal(&windowcoord);
					mouseevent.point.x = windowcoord.h;
					mouseevent.point.y = windowcoord.v;					
					mouseevent.time = 0;
					mouseevent.isShift = FALSE;
					mouseevent.isControl = FALSE;
					mouseevent.isAlt = FALSE;
					mouseevent.clickCount = 1;
					mouseevent.eventStructType = NS_MOUSE_EVENT;
					thewindow->DispatchMouseEvent(mouseevent);
					}
				break;
			}
		}
	gGrabWindow = nsnull;		// mouse grab no longer in effect
}

//==============================================================

void 
nsMacMessagePump::DoMouseMove(EventRecord *aTheEvent)
{
WindowPtr			whichwindow;
PRInt16				partcode;
Point					windowcoord;
nsWindow			*thewindow,*lastwindow;
nsMouseEvent	mouseevent;


	if (*(long*)&mMousePoint == *(long*)&aTheEvent->where)
		return;

	mouseevent.time = 0;
	mouseevent.isShift = FALSE;
	mouseevent.isControl = FALSE;
	mouseevent.isAlt = FALSE;
	mouseevent.clickCount = 1;
	mouseevent.eventStructType = NS_MOUSE_EVENT;
	lastwindow = this->GetCurrentWindow();
	mMousePoint = aTheEvent->where;

	partcode = FindWindow(aTheEvent->where,&whichwindow);
	thewindow = nsnull;
	
	if(whichwindow!=nsnull)
		{
		SetPort(whichwindow);
		thewindow = (nsWindow*)(((WindowPeek)whichwindow)->refCon);
		}
	if( (thewindow != nsnull))
		thewindow = thewindow->FindWidgetHit(aTheEvent->where);


	if(gGrabWindow)
		{
		if( /*(gGrabWindow==thewindow) ||*/ thewindow==lastwindow)
			{
			// JUST A MOUSE MOVE
			mouseevent.message = NS_MOUSE_MOVE;
			mouseevent.widget  = (nsWindow *) gGrabWindow;
			windowcoord = aTheEvent->where;
			GlobalToLocal(&windowcoord);
			mouseevent.point.x = windowcoord.h;
			mouseevent.point.y = windowcoord.v;				
			gGrabWindow->DispatchMouseEvent(mouseevent);
			this->SetCurrentWindow(thewindow);
			}
		else
			{
			// EXIT or ENTER?
			if(lastwindow == nsnull || thewindow != lastwindow)
				{
				if(lastwindow == gGrabWindow)
					{
					mouseevent.message = NS_MOUSE_EXIT;
					mouseevent.widget  = (nsWindow *) gGrabWindow;
					windowcoord = aTheEvent->where;
					GlobalToLocal(&windowcoord);
					mouseevent.point.x = windowcoord.h;
					mouseevent.point.y = windowcoord.v;					
					gGrabWindow->DispatchMouseEvent(mouseevent);	
					this->SetCurrentWindow(thewindow);				
					}
				else
					{
					mouseevent.message = NS_MOUSE_ENTER;
					mouseevent.widget  = (nsWindow *) gGrabWindow;
					windowcoord = aTheEvent->where;
					GlobalToLocal(&windowcoord);
					mouseevent.point.x = windowcoord.h;
					mouseevent.point.y = windowcoord.v;					
					gGrabWindow->DispatchMouseEvent(mouseevent);
					this->SetCurrentWindow(thewindow);				
					}
				}
			}
		return;
		}


	//partcode = FindWindow(aTheEvent->where,&whichwindow);
	switch(partcode)
		{
		case inContent:
			//thewindow = nsnull;
			//if(whichwindow!=nsnull)
				//{
				//SetPort(whichwindow);
				//thewindow = (nsWindow*)(((WindowPeek)whichwindow)->refCon);
				//}
			//if( (thewindow != nsnull))
				//thewindow = thewindow->FindWidgetHit(aTheEvent->where);
				
			if(thewindow)
				{
				if(lastwindow == nsnull || thewindow != lastwindow)
					{
					// mouseexit
					if(lastwindow != nsnull)
						{
						mouseevent.message = NS_MOUSE_EXIT;
						mouseevent.widget  = (nsWindow *) lastwindow;
						windowcoord = aTheEvent->where;
						GlobalToLocal(&windowcoord);
						mouseevent.point.x = windowcoord.h;
						mouseevent.point.y = windowcoord.v;					
						lastwindow->DispatchMouseEvent(mouseevent);
						}

					// mouseenter
					this->SetCurrentWindow(thewindow);
					mouseevent.message = NS_MOUSE_ENTER;
					mouseevent.widget  = (nsWindow *) thewindow;
					windowcoord = aTheEvent->where;
					GlobalToLocal(&windowcoord);
					mouseevent.point.x = windowcoord.h;
					mouseevent.point.y = windowcoord.v;					
					thewindow->DispatchMouseEvent(mouseevent);	
					}
				else
					{
					// mousedown inside the content region
					mouseevent.message = NS_MOUSE_MOVE;
					mouseevent.widget  = (nsWindow *) thewindow;
					windowcoord = aTheEvent->where;
					GlobalToLocal(&windowcoord);
					mouseevent.point.x = windowcoord.h;
					mouseevent.point.y = windowcoord.v;		
					thewindow->DispatchMouseEvent(mouseevent);
					break;
					}
				}
			break;
		default:
			if(lastwindow != nsnull)
				{
				this->SetCurrentWindow(nsnull);
				mouseevent.message = NS_MOUSE_EXIT;
				mouseevent.widget  = (nsWindow *) lastwindow;
				windowcoord = aTheEvent->where;
				GlobalToLocal(&windowcoord);
				mouseevent.point.x = windowcoord.h;
				mouseevent.point.y = windowcoord.v;					
				lastwindow->DispatchMouseEvent(mouseevent);
				}
			break;
		}

}

//==============================================================

void 
nsMacMessagePump::DoKey(EventRecord *aTheEvent)
{
char				ch;
WindowPtr		whichwindow;

	ch = (char)(aTheEvent->message & charCodeMask);
	if(aTheEvent->modifiers&cmdKey)
		{
		// do a menu key command
		}
	else
		{
		whichwindow = FrontWindow();
		if(whichwindow)
			{
			// generate a keydown event for the widget
			}
		}
}

//==============================================================
