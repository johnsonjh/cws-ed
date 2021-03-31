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
|Routine: scroll_down
|Callby: bigger_win change_case fix_botrec fix_display remove_win smaller_win
|Purpose: Scrolls the current window down if possible. Returns the number
|         of lines actually scrolled.
|Arguments:
|    nscroll is the number of lines to try to scroll.
\******************************************************************************/
Int scroll_down(nscroll)
register Int nscroll;
{
	register Int i,j;
	register rec_ptr r;

	if(!nscroll)
		return(0);
	for(r = TOPREC,i = nscroll;r->prev != BASE && i > 0;i--,r = r->prev)
		TOPREC = TOPREC->prev;
	if((i = nscroll - i) > 0)	/* actual number scrollable */
	{
		if(i > BOTROW - TOPROW)
			ref_window(CURWINDOW);
		else
		{
			move(TOPROW,1);
			ins_line(i);
			paint(TOPROW,TOPROW + i - 1,FIRSTCOL);
		}
		for(BOTREC = TOPREC,j = TOPROW;j < BOTROW;j++)
		{
			if(BOTREC == BASE)
			{
				BOTREC = 0;
				break;
			}
			else
				BOTREC = BOTREC->next;
		}
	}
	return(i);
}

