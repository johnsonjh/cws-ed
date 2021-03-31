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

#include <string.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: move_eol
|Callby: edit
|Purpose: Moves the cursor to the ends of records.
|Arguments:
|    repeat is the number of records to move the cursor.
\******************************************************************************/
void move_eol(repeat)
register Int repeat;
{
	if(DIRECTION > 0)
		while(repeat-- > 0)
		{
			if(CURREC == BASE)
			{
				CURBYT = 0;
				abort_key();
				break;
			}
			if(CURBYT == CURREC->length)
			{
				CURREC = CURREC->next;
				CURBYT = CURREC->length;
				CURROW++;
			}
			else
				CURBYT = CURREC->length;
		}
	else	/* direction is backwards */
		while(repeat-- > 0)
		{
			if(CURREC->prev == BASE)
			{
				abort_key();
				break;
			}
			CURREC = CURREC->prev;
			CURBYT = CURREC->length;
			CURROW--;
		}
	CURCOL = get_column(CURREC,CURBYT);
	WANTCOL = CURCOL;
	fix_display();
}

