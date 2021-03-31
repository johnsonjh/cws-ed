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
#include "buffer.h"

/******************************************************************************\
|Routine: buffer_append
|Callby: command copier killer output_file
|Purpose: Appends a record or a portion of a record to an existing buffer.
|Arguments:
|    buf is the existing buffer.
|    rec is the record containing the data to be appended.
|    byt1,byt2 define the portion of the record that is to be appended.
\******************************************************************************/
void buffer_append(buf,rec,byt1,byt2)
register buf_ptr buf;
register rec_ptr rec;
register Int byt1,byt2;
{
	rec_ptr new;
	Int l;

	new = (rec_ptr)imalloc(sizeof(rec_node));
	if((l = byt2 - byt1) > 0)
	{
		new->data = (Char *)imalloc(l + 1);
		memcpy(new->data,rec->data + byt1,l);
		new->data[l] = '\0';
		new->recflags = 1;	/* it is a freeable buffer */
	}
	else
		new->data = NULL;
	new->length = l;
	insq(new,buf->last);
}

