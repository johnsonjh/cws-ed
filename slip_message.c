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
#include <string.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "handy.h"

#define MAXSLIPS 32

static Int pop = 0;
static Char *slips[MAXSLIPS],msg_enabled = 0;

/******************************************************************************\
|Routine: set_wait_main
|Callby: main
|Purpose: Enables or disables "Press any key to continue" messages.
|Arguments:
|    enabled is 1 to enable waiting, 0 to disable it.
\******************************************************************************/
void set_wait_main(enabled)
Int enabled;
{
	msg_enabled = enabled;
}

/******************************************************************************\
|Routine: slip_message
|Callby: command help help_load include_file load_key match_search output_file parse_com set_param show_param store_param wincom
|Purpose: Displays a message at the bottom of the screen.
|Arguments:
|    message is the message to display.
\******************************************************************************/
void slip_message(message)
Char *message;
{
	if(pop < MAXSLIPS)
	{
		slips[pop] = (Char *)imalloc(strlen(message) + 1);
		strcpy(slips[pop++],message);
	}
}

/******************************************************************************\
|Routine: wait_message
|Callby: command help help_load include_file load_key match_search output_file parse_com set_param show_param store_param wincom
|Purpose: Waits for the user to type a key to make sure (s)he sees the message, then repaints the affected area.
|Arguments:
|    none
\******************************************************************************/
void wait_message()
{
	Schar key;
	Int row,toprow,i,popstart;

	reverse();
	if(!NWINDOWS)	/* if there are no windows, just spit the messages out */
	{
		move(BOTROW,1);
		for(i = 0;i < pop;i++)
		{
			unmatchable(BOTROW,BOTROW);
			putz(slips[i]);
			next();
			ifree(slips[i]);
		}
		pop = 0;
		if(msg_enabled)
		{
			unmatchable(BOTROW,BOTROW);
			putz("Press any key to continue...");
			get_next_key(&key);
		}
		normal();
		return;
	}
/* windows are present, so cleanup is required after user types a key */
	popstart = 0;
	do {
		if(pop >= NROW)
			toprow = row = 1;
		else if(pop >= BOTROW)
			toprow = row = NROW - pop;
		else
			toprow = row = BOTROW - pop;
		for(i = 0;i < min(pop,NROW-1);i++)
		{
			move(row++,1);
			ers_end();
			putz(slips[i+popstart]);
			ifree(slips[i+popstart]);
		}
		pop -= i;
		popstart += i;
		move(row,1);
		ers_end();
		putz("Press any key to continue...");
		get_next_key(&key);
		if(pop > 0)
			ers_screen();
	} while (pop > 0);
	normal();
	pop = 0;
	if(popstart >= NROW)
	{
		ref_display();
		return;
	}

	for(i = 0;i < NWINDOWS;i++)
		if(toprow >= WINDOW[i].toprow - 1 && toprow <= WINDOW[i].botrow)
			break;
	if(i == NWINDOWS)	/* Oh hell, do entire screen and hope */
	{
		ref_display();
		return;
	}
	if(toprow == WINDOW[i].toprow - 1)
		ref_window(i);
	else
	{
		save_window();	/* paint_window may need this if i == CURWINDOW */
		paint_window(i,toprow,WINDOW[i].botrow,WINDOW[i].firstcol);
	}
	while(++i < NWINDOWS && WINDOW[i].toprow <= row)
		ref_window(i);
}

