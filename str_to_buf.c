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
#include <string.h>
#include <stdlib.h>

#include "rec.h"
#include "buffer.h"
#include "buf_dec.h"	/* kill buffers */

/******************************************************************************\
|Routine: string_to_buf
|Callby: do_grep journal parse_fnm
|Purpose: Copies a string to a buffer.
|Arguments:
|	string is the string.
|	buf is the buffer.
\******************************************************************************/
void string_to_buf(string,buf)
register Char *string;
register buf_ptr buf;
{
	register rec_ptr r;
	register Int l;

	buffer_empty(buf);
	if((l = strlen(string)))
	{
		r = (rec_ptr)imalloc(sizeof(rec_node));
		r->data = (Char *)imalloc(l + 1);
		strcpy(r->data,string);
		r->length = l;
		r->recflags = 1;	/* it is a freeable buffer */
		insq(r,buf->first);	/* this works because buf->first = &buf->first after call to buffer_empty */
		buf->nrecs = buf->direction = 1;
	}
}

