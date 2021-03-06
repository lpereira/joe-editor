/* GPM/xterm mouse functions
   Copyright (C) 1999 Jesse McGrew

This file is part of JOE (Joe's Own Editor)

JOE is free software; you can redistribute it and/or modify it under the 
terms of the GNU General Public License as published by the Free Software 
Foundation; either version 1, or (at your option) any later version.  

JOE is distributed in the hope that it will be useful, but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
details.  

You should have received a copy of the GNU General Public License along with 
JOE; see the file COPYING.  If not, write to the Free Software Foundation, 
675 Mass Ave, Cambridge, MA 02139, USA.  */ 

#ifndef _Imouse
#define _Imouse 1

#include <sys/time.h>

/* maximum number of milliseconds that can elapse between
   double/triple clicks */
#define MOUSE_MULTI_THRESH	300

#ifdef MOUSE_GPM
int gpmopen();		/* initialize the connection. returns 0 on failure. */
void gpmclose();	/* close the connection. */
#endif

void mouseopen();	/* initialize mouse */
void mouseclose();	/* de-initialize mouse */

/* mousedn(int x, int y) - handle a mouse-down event */
void mousedn(int x, int y);

/* mouseup(int x, int y) - handle a mouse-up event */
void mouseup(int x, int y);

/* mousedrag(int x, int y) - handle a mouse drag event */
void mousedrag(int x, int y);

/* user command handlers */
int uxtmouse(BW *);		/* handle an xterm mouse control sequence */
int utomouse(BW *);		/* move the pointer to the mouse */
int udefmdown(BW *);	/* default mouse click handlers */
int udefmup(BW *);
int udefmdrag(BW *);
int udefm2down(BW *);
int udefm2up(BW *);
int udefm2drag(BW *);
int udefm3down(BW *);
int udefm3up(BW *);
int udefm3drag(BW *);

time_t mnow();
void reset_trig_time();

/* options */
extern int floatmouse;	/* Allow mouse to set cursor past end of lines */
extern int rtbutton; /* Use button 3 instead of button 1 */

extern int auto_scroll; /* Set for autoscroll */
extern time_t auto_trig_time; /* Time of next scroll */
extern int joexterm; /* Set if xterm can do base64 paste */

#endif
