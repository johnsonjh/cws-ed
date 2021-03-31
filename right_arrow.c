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

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: right_arrow
|Callby: edit insert wincom word_fill
|Purpose: Does what is required to move the cursor to the right. If the cursor
|         is at the end of a record, moves it to the beginning of the following
|         record.
|Arguments:
|    repeat is the repeat count for the action.
\******************************************************************************/
void right_arrow(repeat)
register Int repeat;
{
	Int i;
	register Int ch,c;
	register Char *p;

	i = repeat;
	while(CURREC->length - CURBYT < i)
	{
		if(CURREC == BASE)
		{
			i = 0;
			abort_key();
			break;
		}
		i -= CURREC->length - CURBYT + 1;
		CURBYT = 0;
		CURREC = CURREC->next;
		CURROW++;
		CURCOL = 1;
	}
	if(HEXMODE)
	{
		CURCOL += (i << 2);
		CURBYT += i;
	}
	else
	{
		for(c = CURCOL - 1,p = CURREC->data + CURBYT;i--;p++,CURBYT++)
		{
			if((ch = *p) == '\t')
				c = tabstop(c);
			else if(NONPRINTNCS(ch))
				c += 4;
			else
				c++;
		}
		CURCOL = ++c;
	}
	WANTCOL = CURCOL;
	fix_display();
}

