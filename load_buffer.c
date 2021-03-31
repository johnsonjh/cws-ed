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
|Routine: load_buffer
|Callby: edit word_fill
|Purpose: Loads a buffer with the contents of a string, handling appearances
|         of <cr> in the string by starting a new record in the buffer.
|Arguments:
|    buffer is the buffer that receives the record(s) from the string.
|    string is the string that may contain <cr> characters.
|    i is the length of the meaningful data in the string.
\******************************************************************************/
void load_buffer(buffer,string,i)
buf_ptr buffer;
Char *string;
Int i;
{
	register Int j,l;
	register rec_ptr r;

	buffer_empty(buffer);
	string[i++] = '\r';
	buffer->nrecs = 0;
	for(l = j = 0;j < i;j++)
		if(string[j] == '\r')
		{
			buffer->nrecs++;
			r = (rec_ptr)imalloc(sizeof(rec_node));
			r->data = (Char *)imalloc(j - l + 1);
			if(j > l)
				memcpy(r->data,string + l,j - l);
			r->data[j - l] = '\0';
			r->length = j - l;
			r->recflags = 1;	/* it is a freeable buffer */
			insq(r,buffer->last);
			l = j + 1;
		}
}

