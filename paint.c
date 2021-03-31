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
|Routine: paint
|Callby: cfrendly change_case command edit insert killer parse_com ref_window remove_win scroll_down scroll_up show_param slip_message wincom
|Purpose: Redisplays lines on the screen.
|Arguments:
|    top is the top line to redisplay.
|    bot is the bottom line to redisplay.
|    fcol is the column number of the leftmost column on the screen (reflects
|            leftward shift of display).
\******************************************************************************/
void paint(top,bot,fcol)
register Int top,bot,fcol;
{
	register rec_ptr r;
	register Int row,l,selcol;
	Char *buf;

	if(!puttest())
		return;
	r = TOPREC;
	row = TOPROW;
	while(row < top && r != BASE)
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
	while(row <= bot && r != BASE)
	{
		if(r->recflags & 2)
			reverse();
		l = express(r->length,r->data,TAB_STRING[CUR_TAB_SETUP],TAB_LENGTH[CUR_TAB_SETUP],&buf,fcol + NCOL,0);
		if(l >= fcol)
			putz(buf + fcol - 1);
		if(l < NCOL + fcol - 1)
			ers_end();
		if(r->recflags & 2)
			normal();
		if(r == SELREC)
		{
			selcol = get_column(r,SELBYT) - FIRSTCOL + 1;
			if(selcol > 0 && selcol <= NCOL)
			{
				cr();
				if(selcol > 1)
					right(selcol - 1);
				doselect(r,SELBYT);
			}
		}
		r = r->next;
		row++;
		if(row <= bot)
			next();
	}
	if(r == BASE && row <= bot)
	{
		ers_end();
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
}

