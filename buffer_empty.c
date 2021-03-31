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
#include "buffer.h"

/******************************************************************************\
|Routine: buffer_empty
|Callby: carriage_ret command copier include_file inquire killer load_buffer openline output_file sort_recs str_to_buf word_fill
|Purpose: Removes all records from a buffer and frees them.
|Arguments:
|    buffer is the buffer to be emptied.
\******************************************************************************/
void buffer_empty(buffer)
buf_ptr buffer;
{
	register rec_ptr e,next,b;

	for(b = buffer->first,e = (rec_ptr)&buffer->first;b != e;)
	{
		next = b->next;
		remq(b);
		if(b->data && (b->recflags & 1))
			ifree(b->data);
		ifree(b);
		b = next;
	}
	buffer->nrecs = 0;
}

