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
#include "handy.h"

/******************************************************************************\
|Routine: paint_window
|Callby: ref_window slip_message
|Purpose: Redisplays lines not in the current window.
|Arguments:
|    w is the number of the window in which the lines will be redisplayed.
|    top is the top line to redisplay.
|    bot is the bottom line to redisplay.
|    fcol is the column number of the leftmost column on the screen (reflects
|            leftward shift of display).
\******************************************************************************/
void paint_window(w,top,bot,fcol)
register Int w,top,bot,fcol;
{
	register rec_ptr r;
	register Int row,l,selcol;
	Int save_hexmode;
	Char *buf;

	r = WINDOW[w].toprec;
	row = WINDOW[w].toprow;
	while(row < top && r != WINDOW[w].base)
	{
		r = r->next;
		row++;
	}
	move(top,1);
	if(row < top)
	{
		for(l = top;l <= bot;l++)
		{
			move(l,1);
			ers_end();
		}
		return;
	}
	save_hexmode = HEXMODE;
	HEXMODE = (WINDOW[w].binary == 2);
	while(row <= bot && r != WINDOW[w].base)
	{
		if(r->recflags & 2)
			reverse();
		l = express(r->length,r->data,TAB_STRING[CUR_TAB_SETUP],TAB_LENGTH[CUR_TAB_SETUP],&buf,fcol + NCOL,0);
		if(l >= fcol)
			putz(buf + fcol - 1);
		if(r->recflags & 2)
			normal();
		if(r == SELREC)
		{
			if((selcol = get_column(r,SELBYT) - WINDOW[w].firstcol + 1) > 0)
			{
				cr();
				if(selcol > 1)
					right(selcol - 1);
				doselect(r,SELBYT);
			}
			if((selcol = get_column(r,r->length) - WINDOW[w].firstcol + 1) > 0)
				move(row,(SELBYT == r->length)? selcol + 1 : selcol);
		}
		if(l < NCOL + fcol - 1)
			ers_end();
		r = r->next;
		row++;
		if(row <= bot)
			next();
	}
	if(r == WINDOW[w].base && row <= bot)
	{
		ers_end();
		if(fcol == 1)
		if(fcol == 1)
		{
			eob();
			if(r == SELREC)
			{
				cr();
				doselect(r,0);
			}
		}
		while(row++ < bot)
		{
			next();
			ers_end();
		}
	}
	HEXMODE = save_hexmode;
}

