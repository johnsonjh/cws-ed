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

static Int marked = 0,row,col;
static Char buf[2],enabled = 1;

/******************************************************************************\
|Routine: match_paren
|Callby: insert
|Purpose: Handles parenthesis matching.
|Arguments:
|    open is the open-parenthesis character.
|    close is the close-parenthesis character.
|    dir is the direction to search while looking for the match.
\******************************************************************************/
void match_paren(open,close,dir)
register Char open,close;
register Int dir;
{
	register Int level,byt;
	register rec_ptr rec;
	register Char c;

	if(enabled)
	{
		row = CURROW;
		rec = CURREC;
		level = 1;
		byt = CURBYT - 1;
		if(dir < 0)
			while(level > 0)
			{
				if(--byt < 0)
				{
					rec = rec->prev;
					if(--row < TOPROW)	/* this will catch hitting top of file */
						return;
					byt = rec->length - 1;
				}
				if((c = rec->data[byt]) == close)
					level++;
				else if(c == open)
					level--;
			}
		else
			while(level > 0)
			{
				if(++byt >= rec->length)
				{
					if((rec = rec->next) == BASE)
						return;
					if(++row > BOTROW)
						return;
					byt = 0;
				}
				if((c = rec->data[byt]) == close)
					level--;
				else if(c == open)
					level++;
			}
		col = get_column(rec,byt);
		move(row,col);
		reverse();
		buf[0] = c;
		buf[1] = '\0';
		putz(buf);
		normal();
		move(CURROW,CURCOL);
		marked = 1;
	}
}

/******************************************************************************\
|Routine: unmark_paren
|Callby: edit
|Purpose: Removes the marking on a matched parenthesis.
|Arguments:
|    none
\******************************************************************************/
void unmark_paren()
{
	if(marked)
	{
		move(row,col);
		putz(buf);
		move(CURROW,CURCOL);
		marked = 0;
	}
}

/******************************************************************************\
|Routine: toggle_paren
|Callby: edit
|Purpose: Disables marking of matched parentheses, and reenables it on a second call.
|Arguments:
|    none
\******************************************************************************/
void toggle_paren()
{
	enabled = !enabled;
}

