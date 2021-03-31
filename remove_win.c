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

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: remove_window
|Callby: wincom
|Purpose: Removes a window from the screen. Throws away everything in the window.
|Arguments:
|    n is the number of the window to remove (zero-based).
\******************************************************************************/
void remove_window(n)
Int n;
{
	Int i,central,oldbot,oldtop,oldwin;
	rec_ptr r;

	oldwin = CURWINDOW;	/* save so we can move to correct window after all is done */
#ifndef GNUDOS
	update_bookmark(n);
#endif
	toss_data(WINDOW[n].base);	/* toss all the records, and the base pointer */
	ifree(WINDOW[n].filename);	/* toss the file name buffer */
	if(WINDOW[n].filebuf)
		unmap_file(WINDOW[n].filebuf);
	if(WINDOW[n].bookmark)
		ifree(WINDOW[n].bookmark);	/* toss the bookmark file name buffer */
	WINDOW[n].bookmark = NULL;
	if(n == NWINDOWS - 1)	/* closing bottom window */
	{
		i = --NWINDOWS - 1;
		oldbot = WINDOW[i].botrow;
		WINDOW[i].botrow = NROW;
		new_botrec(i);
		calc_limits(WINDOW[i].toprow,WINDOW[i].botrow,&WINDOW[i].toplim,&WINDOW[i].botlim);
		set_window(i);
		paint(oldbot + 1,NROW,FIRSTCOL);
		if(CURROW < TOPLIM)
			CURROW += scroll_down(TOPLIM - CURROW);
		fix_botrec();
		save_window();
	}
	else
	{
/* closing an enclosed window */
		central = (WINDOW[n].toprow + WINDOW[n].botrow) >> 1;	/* this becomes filename line of follower */
		oldbot = WINDOW[n - 1].botrow;
		WINDOW[n - 1].botrow = central - 1;
		new_botrec(n - 1);
		calc_limits(WINDOW[n - 1].toprow,WINDOW[n - 1].botrow,&WINDOW[n - 1].toplim,&WINDOW[n - 1].botlim);
		set_window(n - 1);
		paint(oldbot + 1,central - 1,FIRSTCOL);
		if(CURROW < TOPLIM)
			CURROW += scroll_down(TOPLIM - CURROW);
		fix_botrec();
		save_window();
/* now expand the following window */
		move(central,1);
		ers_end();
		reverse();
		putz(WINDOW[n + 1].filename);
		normal();
		oldtop = WINDOW[n + 1].toprow;
		WINDOW[n + 1].toprow = central + 1;
		calc_limits(WINDOW[n + 1].toprow,WINDOW[n + 1].botrow,&WINDOW[n + 1].toplim,&WINDOW[n + 1].botlim);
/* we have to be more careful when expanding upwards... */
		r = WINDOW[n + 1].toprec;
		for(i = oldtop;i > WINDOW[n + 1].toprow && r->prev != WINDOW[n + 1].base;i--,r = r->prev);
		WINDOW[n + 1].toprec = r;
		if(i == WINDOW[n + 1].toprow)	/* we were able to back up enough */
		{
			set_window(n + 1);
			paint(central + 1,oldtop - 1,FIRSTCOL);
		}
		else	/* had to scroll some... */
		{
			new_botrec(n + 1);
			set_window(n + 1);
			paint(TOPROW,BOTROW,FIRSTCOL);
			CURROW -= i - TOPROW;
		}
		if(CURROW > BOTLIM)
			CURROW -= scroll_up(CURROW - BOTLIM);
		fix_botrec();
		save_window();
/* move the window data structures around */
		NWINDOWS--;
		for(i = n;i < NWINDOWS;i++)
		{
			WINDOW[i].base = WINDOW[i + 1].base;
			WINDOW[i].currec = WINDOW[i + 1].currec;
			WINDOW[i].toprec = WINDOW[i + 1].toprec;
			WINDOW[i].botrec = WINDOW[i + 1].botrec;
			WINDOW[i].curbyt = WINDOW[i + 1].curbyt;
			WINDOW[i].toprow = WINDOW[i + 1].toprow;
			WINDOW[i].botrow = WINDOW[i + 1].botrow;
			WINDOW[i].toplim = WINDOW[i + 1].toplim;
			WINDOW[i].botlim = WINDOW[i + 1].botlim;
			WINDOW[i].currow = WINDOW[i + 1].currow;
			WINDOW[i].curcol = WINDOW[i + 1].curcol;
			WINDOW[i].firstcol = WINDOW[i + 1].firstcol;
			WINDOW[i].wantcol = WINDOW[i + 1].wantcol;
			WINDOW[i].modified = WINDOW[i + 1].modified;
			WINDOW[i].diredit = WINDOW[i + 1].diredit;
			WINDOW[i].binary = WINDOW[i + 1].binary;
			WINDOW[i].news = WINDOW[i + 1].news;
			WINDOW[i].filebuf = WINDOW[i + 1].filebuf;
			WINDOW[i].filename = WINDOW[i + 1].filename;
			WINDOW[i].bookmark = WINDOW[i + 1].bookmark;
		}
	}
	if(n <= oldwin)
		oldwin--;
	set_window(oldwin);
	move(CURROW,CURCOL);
}

