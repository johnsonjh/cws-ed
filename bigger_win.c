/*
 * Copyright (C) 1992 by Rush Record
 * Copyright (C) 1993 by Charles Sandmann (sandmann@clio.rice.edu)
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
|Routine: bigger_window
|Callby: command
|Purpose: Increases the size of a window.
|Arguments:
|    increase - the number of lines to add to the window.
\******************************************************************************/
void bigger_window(increase)
Int increase;
{
	Int i,j,space,topavail,botavail,topgrab,botgrab,excess,trim;
	rec_ptr r;
	
	if(NWINDOWS == 1)
		return;
	save_window();
/* get the amount available above and below */
	for(topavail = botavail = i = 0;i < NWINDOWS;i++)
		if((excess = WINDOW[i].botrow - WINDOW[i].toprow - 2))
		{
			if(i < CURWINDOW)
				topavail += excess;
			if(i > CURWINDOW)
				botavail += excess;
		}
	increase = min(increase,topavail + botavail);
/* allocation algorithm CWS 10-93; split equally, then fix for availability */
	topgrab = increase >> 1;
	botgrab = increase - topgrab;
	/* if not enough room to expand bottom, glom it with top */
	if((i = botgrab - botavail) > 0)
	{
		topgrab += i;
		botgrab = botavail;
	}
	else if((i = topgrab - topavail) > 0)
	{
		topgrab = topavail;
		botgrab += i;
	}
	if((space = topgrab))
	{
		for(i = CURWINDOW - 1;i >= 0;i--)	/* start with previous window */
		{
			if((excess = WINDOW[i].botrow - WINDOW[i].toprow - 2))
			{
				trim = min(space,excess);
				if(i < CURWINDOW - 1)	/* del_line not necessary on first borrow, ref_window takes care of that */
				{
					marge(1,NROW);
					move(WINDOW[i].botrow - trim + 1,1);
					del_line(trim);
					move(WINDOW[CURWINDOW].toprow,1);
					ins_line(trim);
				}
				WINDOW[CURWINDOW].toprow -= trim;
				WINDOW[CURWINDOW].currow -= trim;
				WINDOW[i].botrow -= trim;
				new_botrec(i);
				calc_limits(WINDOW[i].toprow,WINDOW[i].botrow,&WINDOW[i].toplim,&WINDOW[i].botlim);
				if(WINDOW[i].currow > WINDOW[i].botlim)
				{
					j = CURWINDOW;
					set_window(i);
					CURROW -= scroll_up(CURROW - BOTLIM);
					save_window();
					set_window(j);
				}
				for(j = i + 1;j < CURWINDOW;j++)
				{
					WINDOW[j].toprow -= trim;
					WINDOW[j].botrow -= trim;
					WINDOW[j].toplim -= trim;
					WINDOW[j].botlim -= trim;
					WINDOW[j].currow -= trim;
				}
				if(!(space -= trim))
					break;
			}
		}
	}
	if((space = botgrab))
	{
		for(i = NWINDOWS - 1;i > CURWINDOW;i--)	/* start with bottom window */
		{
			if((excess = WINDOW[i].botrow - WINDOW[i].toprow - 2))
			{
				trim = min(space,excess);
				marge(1,NROW);
				move(WINDOW[CURWINDOW].botrow + 1,1);
				ins_line(trim);
				WINDOW[CURWINDOW].botrow += trim;
				WINDOW[i].toprow += trim;
				WINDOW[i].currow += trim;
				new_botrec(i);
				calc_limits(WINDOW[i].toprow,WINDOW[i].botrow,&WINDOW[i].toplim,&WINDOW[i].botlim);
				if(WINDOW[i].currow > WINDOW[i].botlim)
				{
					j = CURWINDOW;
					set_window(i);
					CURROW -= scroll_up(CURROW - BOTLIM);
					save_window();
					set_window(j);
				}
				for(j = i - 1;j > CURWINDOW;j--)
				{
					WINDOW[j].toprow += trim;
					WINDOW[j].botrow += trim;
					WINDOW[j].toplim += trim;
					WINDOW[j].botlim += trim;
					WINDOW[j].currow += trim;
				}
				if(!(space -= trim))
					break;
			}
		}
	}
	new_botrec(CURWINDOW);
	calc_limits(WINDOW[CURWINDOW].toprow,WINDOW[CURWINDOW].botrow,&WINDOW[CURWINDOW].toplim,&WINDOW[CURWINDOW].botlim);
	set_window(CURWINDOW);
	ref_window(CURWINDOW);
	fix_display();
	if(!BOTREC)
	{
		for(i = 0,r = TOPREC;r != BASE;r = r->next,i++);
		CURROW += scroll_down(BOTROW - TOPROW - i);
	}
}

