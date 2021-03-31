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
#include <stdlib.h>

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"

/******************************************************************************\
|Routine: carriage_return
|Callby: cfrendly edit insert word_fill
|Purpose: To do what is required when the user hits carriage return while
|         editing. This always creates a new record, and usually splits an
|         existing record. This routine also implements insertion of tabs in
|         the new record to make it line up with the previous record, if the
|         TAB_AUTO flag is set.
|Arguments:
|    repeat is the repeat count applied to the action.
\******************************************************************************/
void carriage_return(repeat)
Int repeat;
{
	buf_node cr;
	register rec_ptr r;
	register Int i,ntabs;

/* count leading tabs in previous record */
	ntabs = 0;
	if(CURREC != BASE && TAB_AUTO && !OVERSTRIKE) /* CWS 11-93 over bug */
	{
		for(ntabs = 0;CURREC->data[ntabs] == '\t' || CURREC->data[ntabs] == ' ';ntabs++);
		if(CURBYT < ntabs)
			ntabs = CURBYT;
	}
/* set up a buffer to hold the new record(s), and create and insert the new record(s) */
	cr.nrecs = repeat + 1;
	cr.direction = 1;
	cr.first = (rec_ptr)&cr.first;
	cr.last = (rec_ptr)&cr.first;
	for(i = 0;i <= repeat;i++)
	{
		r = (rec_ptr)imalloc(sizeof(rec_node));
		if((r->length = (!i)? 0 : ntabs))
		{
			r->data = (Char *)imalloc(ntabs + 1);
			memcpy(r->data,CURREC->data,ntabs);
			r->recflags = 1;	/* it is a freeable buffer */
		}
		else
			r->data = NULL;
		insq(r,cr.last);
	}
/* put it all in the file, and free storage */
	insert(&cr);
	buffer_empty(&cr);
}

