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
|Routine: up_arrow
|Callby: backspace copier edit insert killer move_line
|Purpose: Does what is required to move the cursor up.
|Arguments:
|    repeat is the repeat count for the action.
\******************************************************************************/
void up_arrow(repeat)
register Int repeat;
{
	register Int i;

	for(i = repeat;CURREC->prev != BASE && i > 0;i--,CURROW--)
		CURREC = CURREC->prev;
	if(i != 0)
		abort_key();
	i = get_column(CURREC,CURREC->length);
	if(WANTCOL >= i)
	{
		CURCOL = i;
		CURBYT = CURREC->length;
	}
	else
		CURCOL = find_column(CURREC,WANTCOL,&CURBYT);
	fix_display();
}

