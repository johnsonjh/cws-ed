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

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: scroll_up
|Callby: bigger_win change_case fix_display insert_win remove_win
|Purpose: Scrolls the current window up if possible. Returns the number
|         of lines actually scrolled.
|Arguments:
|    nscroll is the number of lines to try to scroll.
\******************************************************************************/
Int scroll_up(nscroll)
register Int nscroll;
{
	register Int i;
	register rec_ptr r;

	for(r = BOTREC,i = nscroll;r != BASE && r != 0 && i > 0;i--,r = r->next)
	{
		TOPREC = TOPREC->next;
		if(BOTREC != 0 && BOTREC != BASE)
			BOTREC = BOTREC->next;
	}
	if((i = nscroll - i) > 0)	/* actual number scrollable */
	{
		if(i > BOTROW - TOPROW)
			ref_window(CURWINDOW);
		else
		{
			move(TOPROW,1);
			del_line(i);
			paint(BOTROW - i + 1,BOTROW,FIRSTCOL);
		}
	}
	return(i);
}

