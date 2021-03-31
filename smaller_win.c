/*
 * Copyright (C) 1992 by Rush Record (rhr@clio.rice.edu)
 * 
 * This file is part of ED.
 * 
 * ED is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation.
 * 
 * ED is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with ED
 * (see the file COPYING).  If not, write to the Free Software Foundation, 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */
#include "opsys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "handy.h"

/******************************************************************************\
|Routine: smaller_window
|Callby: command
|Purpose: Decreases the size of a window.
|Arguments:
|    decrease - the number of lines to subtract from the window.
\******************************************************************************/
void smaller_window(decrease)
Int decrease;
{
	Int i,j,topfree,botfree;
	rec_ptr r;
	
	if(NWINDOWS == 1)
		return;
/* don't do anything if the window is 3 lines */
	if((i = BOTROW - TOPROW + 1) <= 3)
		return;
/* don't free so many lines we go below three lines for this window */
	if(i - decrease < 3)
		decrease = i - 3;
/* handle different cases */
	if(!CURWINDOW)	/* must shrink at bottom */
	{
		topfree = 0;
		botfree = decrease;
	}
	else if(CURWINDOW == NWINDOWS - 1)	/* must shrink at top */
	{
		topfree = decrease;
		botfree = 0;
	}
	else	/* split shrinkage equally */
	{
		topfree = decrease >> 1;
		botfree = decrease - topfree;
	}
/* shrink the window */
	save_window();
	if(topfree)
	{
		TOPROW = WINDOW[CURWINDOW].toprow += topfree;
		WINDOW[i = CURWINDOW - 1].botrow += topfree;
		new_botrec(i);
		calc_limits(WINDOW[i].toprow,WINDOW[i].botrow,&WINDOW[i].toplim,&WINDOW[i].botlim);
		marge(1,NROW);
		ref_window(i);
		if(!WINDOW[i].botrec)
		{
			j = CURWINDOW;
			set_window(j - 1);
			for(i = 0,r = TOPREC;r != BASE;r = r->next,i++);
			CURROW += scroll_down(BOTROW - TOPROW - i);
			save_window();
			set_window(j);
		}
	}
	if(botfree)
	{
		BOTROW = WINDOW[CURWINDOW].botrow -= botfree;
		WINDOW[i = CURWINDOW + 1].toprow -= botfree;
		calc_limits(WINDOW[i].toprow,WINDOW[i].botrow,&WINDOW[i].toplim,&WINDOW[i].botlim);
		new_botrec(i);
		for(r = WINDOW[i].toprec,j = WINDOW[i].toprow;j != WINDOW[i].botrow;r = r->next,j++)
		{
			if(r == WINDOW[i].currec)
			{
				WINDOW[i].currow = j;
				break;
			}
			if(r == WINDOW[i].base)
				break;
		}
		marge(1,NROW);
		ref_window(i);
		if(!WINDOW[i].botrec)
		{
			j = CURWINDOW;
			set_window(j + 1);
			for(i = 0,r = TOPREC;r != BASE;r = r->next,i++);
			CURROW += scroll_down(BOTROW - TOPROW - i);
			save_window();
			set_window(j);
		}
	}
	calc_limits(TOPROW,BOTROW,&TOPLIM,&BOTLIM);
	CURROW = i = (TOPLIM + BOTLIM) >> 1;	/* put CURREC midway between limits */
	for(r = CURREC;r->prev != BASE && i != TOPROW;i--,r = r->prev);
	TOPREC = r;
	CURROW -= (i - TOPROW);
	save_window();
	new_botrec(CURWINDOW);
	set_window(CURWINDOW);
	ref_window(CURWINDOW);
	if(!BOTREC)
	{
		for(i = 0,r = TOPREC;r != BASE;r = r->next,i++);
		CURROW += scroll_down(BOTROW - TOPROW - i);
	}
}

