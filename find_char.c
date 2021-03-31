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
|Routine: find_char
|Callby: edit
|Purpose: This routine just checks that the other end of a delete region
|         does not hit the start or end of the buffer.
|Arguments:
|    prepeat is the offset to the other end of the delete region.
\******************************************************************************/
Int find_char(prepeat)
Int prepeat;
{
	register Int curbyt,i;
	register rec_ptr currec;

	curbyt = CURBYT;
	currec = CURREC;
	if(prepeat > 0)
	{
		i = prepeat;
		while(currec->length - curbyt < i)
		{
			if(currec == BASE)
				return(0);
			i -= currec->length - curbyt + 1;
			curbyt = 0;
			currec = currec->next;
		}
	}
	else
	{
		i = -prepeat;
		while(curbyt < i)
		{
			i -= curbyt + 1;
			if(currec->prev == BASE)
				return(0);
			currec = currec->prev;
			curbyt = currec->length;
		}
	}
	return(prepeat);
}

