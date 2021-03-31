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
|Routine: move_line
|Callby: edit
|Purpose: Moves the cursor to the beginnings of lines.
|Arguments:
|    repeat is the number of lines to move the cursor.
\******************************************************************************/
void move_line(repeat)
register Int repeat;
{
	WANTCOL = 1;
	if(DIRECTION < 0)
	{
		backspace(1);
		if(--repeat)
			up_arrow(repeat);
	}
	else
		down_arrow(repeat);
}

/******************************************************************************\
|Routine: find_line
|Callby: edit
|Purpose: Does what move_line does, but instead of actually moving the cursor,
|         returns an offset to the position at which the cursor would be.
|Arguments:
|    prepeat is the number of lines to (figuratively) move.
\******************************************************************************/
Int find_line(prepeat)
Int prepeat;
{
	register Int repeat,curbyt;
	Int offset;
	rec_ptr currec;

	curbyt = CURBYT;
	currec = CURREC;
	offset = 0;
	if(prepeat > 0)
	{
		repeat = prepeat;
		while(repeat-- > 0)
		{
			if(currec == BASE)
				return(0);
			offset += currec->length + 1 - curbyt;
			curbyt = 0;
			currec = currec->next;
		}
	}
	else	/* backwards */
	{
		repeat = -prepeat;
		while(repeat-- > 0)
		{
			if(currec->prev == BASE && !curbyt)
				return(0);
			if(curbyt > 0)
			{
				offset -= curbyt;
				curbyt = 0;
			}
			else
			{
				currec = currec->prev;
				offset -= currec->length + 1;
			}
		}
	}
	return(offset);
}

