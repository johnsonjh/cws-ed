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
|Routine: get_markdir
|Callby: edit
|Purpose: Figures out in which direction, from the cursor position, a particular
|         spot in the file lies.
|Arguments:
|    rec is the record containing the spot.
|    byt is the byte offset in that record.
\******************************************************************************/
Int get_markdir(rec,byt)
rec_ptr rec;
Int byt;
{
	register rec_ptr forw,back;

	if(rec == CURREC)
	{
		if(byt < CURBYT)
			return(-1);
		else if(byt > CURBYT)
			return(1);
		else
			return(0);
	}
	if(rec == BASE)
		return(1);
	if(CURREC == BASE)
		return(-1);
	forw = back = CURREC;
	while(forw != rec && back != rec)
	{
		if((forw = forw->next) == BASE)
			return(-1);
		if((back = back->prev) == BASE)
			return(1);
	}
	if(forw == rec)
		return(1);
	else
		return(-1);
}

