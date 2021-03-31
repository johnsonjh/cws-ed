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
#include <string.h>

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: rec_copy
|Callby: insert
|Purpose: Makes a copy of a record.
|Arguments:
|    rec is the record to copy.
|    pnew is the newly created record.
\******************************************************************************/
void rec_copy(rec,pnew)
rec_ptr rec,*pnew;
{
	rec_ptr new;

	new = (rec_ptr)imalloc(sizeof(rec_node));
	new->data = (Char *)imalloc(rec->length + 1);
	if((new->length = rec->length) > 0)
		memcpy(new->data,rec->data,rec->length);
	new->data[new->length] = '\0';
	new->recflags = 1;	/* it is a freeable buffer */
	*pnew = new;
}

