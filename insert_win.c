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

static Char percentaged = 0;
static Int percent;

/******************************************************************************\
|Routine: insert_window_percent
|Callby: edit news_group
|Purpose: Allows calling code to require that the created window occupy a given
|         percentage of the bottommost window.
|Arguments:
|    perc is the percentage, 0-100, integer.
\******************************************************************************/
void insert_window_percent(perc)
Int perc;
{
	percentaged = 1;
	percent = perc;
}

/******************************************************************************\
|Routine: insert_window
|Callby: command edit wincom
|Purpose: Handles creation of a new window on the screen.
|Arguments:
|    file is the name of the file for the window.
|    base is the base of the record queue for the new window.
|    binary flags that the window is in binary or hex mode.
|    diredit flags that the window is a diredit window.
\******************************************************************************/
void insert_window(file,base,binary,diredit)
Char *file;
rec_ptr base;
Int binary,diredit;
{
	Int i,j,diff,rows,space,excess,trim;

	diff = WINDOW[NWINDOWS - 1].botrow - WINDOW[NWINDOWS - 1].toprow;
	if(percentaged)
	{
		percentaged = 0;
		rows = ((diff + 1) * percent) / 100;
	}
	else
		rows = (diff + 2) >> 1;
	if(rows < 4)
	{
		space = 6 - diff;	/* we have to get this many lines from existing windows */
		for(i = 0;i < NWINDOWS - 1;i++)
			if((excess = WINDOW[i].botrow - WINDOW[i].toprow - 2) > 0)	/* there is space available here, use it */
			{
				trim = min(space,excess);
				marge(1,NROW);
				move(WINDOW[i].botrow - trim + 1,1);
				del_line(trim);
				WINDOW[i].botrow -= trim;
				new_botrec(i);
				calc_limits(WINDOW[i].toprow,WINDOW[i].botrow,&WINDOW[i].toplim,&WINDOW[i].botlim);
				if(WINDOW[i].currow > WINDOW[i].botlim)
				{
					set_window(i);
					CURROW -= scroll_up(CURROW - BOTLIM);
					save_window();
				}
				for(j = i + 1;j < NWINDOWS;j++)
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
		rows = 4;
	}
	i = NWINDOWS - 1;
	WINDOW[i].botrow = NROW - rows;
	new_botrec(i);
	calc_limits(WINDOW[i].toprow,WINDOW[i].botrow,&WINDOW[i].toplim,&WINDOW[i].botlim);
	if(WINDOW[i].currow > WINDOW[i].botlim)
	{
		set_window(i);
		CURROW -= scroll_up(CURROW - BOTLIM);
		save_window();
	}
	WINDOW[NWINDOWS].base = base;
	WINDOW[NWINDOWS].currec = base->next;
	WINDOW[NWINDOWS].toprec = base->next;
	WINDOW[NWINDOWS].curbyt = 0;
	WINDOW[NWINDOWS].toprow = WINDOW[NWINDOWS].currow = NROW - rows + 2;
	WINDOW[NWINDOWS].botrow = NROW;
	new_botrec(NWINDOWS);
	calc_limits(WINDOW[NWINDOWS].toprow,WINDOW[NWINDOWS].botrow,&WINDOW[NWINDOWS].toplim,&WINDOW[NWINDOWS].botlim);
	WINDOW[NWINDOWS].curcol = WINDOW[NWINDOWS].firstcol = WINDOW[NWINDOWS].wantcol = 1;
	WINDOW[NWINDOWS].modified = 0;
	WINDOW[NWINDOWS].filebuf = NULL;
	WINDOW[NWINDOWS].filename = (Char *)imalloc(strlen(file) + 1);
	strcpy(WINDOW[NWINDOWS].filename,file);
	WINDOW[NWINDOWS].bookmark = NULL;
	WINDOW[NWINDOWS].binary = binary;
	WINDOW[NWINDOWS].news = 0;	/* this is the default, newsreading code must change it. */
/* if this is a remote diredit buffer, store in the list for this window */
	if(host_in_name(file) && diredit)
		dir_store(NWINDOWS,file,base);
	set_window(NWINDOWS);
	move(TOPROW,1);
	ers_bottom();
	ref_window(NWINDOWS++);
}

