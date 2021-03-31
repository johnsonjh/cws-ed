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
|Routine: left_arrow
|Callby: edit insert word_fill
|Purpose: Does what is required to move the cursor to the left. If the cursor
|         is at the beginning of a record, moves it to the end of the previous
|         record.
|Arguments:
|    repeat is the repeat count for the action.
\******************************************************************************/
void left_arrow(repeat)
Int repeat;
{
	register Int i;

	i = repeat;
	while(CURBYT < i)
	{
		i -= CURBYT + 1;
		if(CURREC->prev == BASE)
		{
			CURBYT = i;
			abort_key();
			break;
		}
		CURREC = CURREC->prev;
		CURBYT = CURREC->length;
		CURROW--;
	}
	CURBYT -= i;
	CURCOL = get_column(CURREC,CURBYT);
	WANTCOL = CURCOL;
	fix_display();
}

