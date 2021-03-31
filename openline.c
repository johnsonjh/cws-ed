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

#include <stdio.h>
#include <stdlib.h>

#include "rec.h"
#include "window.h"
#include "ed_dec.h"
#include "buffer.h"

/******************************************************************************\
|Routine: openline
|Callby: edit
|Purpose: Inserts new, empty records in the current file.
|Arguments:
|    repeat is the count of records to insert.
\******************************************************************************/
void openline(repeat)
register Int repeat;
{
	buf_node cr;
	rec_ptr r;
	register Int i;

	cr.nrecs = repeat + 1;
	cr.direction = -1;
	cr.first = (rec_ptr)&cr.first;
	cr.last = (rec_ptr)&cr.first;
	for(i = 0;i <= repeat;i++)
	{
		r = (rec_ptr)imalloc(sizeof(rec_node));
		r->length = 0;
		r->data = NULL;
		insq(r,cr.last);
	}
	insert(&cr);
	buffer_empty(&cr);
}

