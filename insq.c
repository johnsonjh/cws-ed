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

/******************************************************************************\
|Routine: insq
|Callby: buffer_app carriage_ret copy_buffer do_grep include_file inquire insert load_buffer load_file openline rec_insert rec_merge rec_split rec_trim str_to_buf
|Purpose: Inserts a record in a record queue.
|Arguments:
|    e is the record to insert.
|    p is the new record's predecessor.
\******************************************************************************/
void insq(e,p)
register rec_ptr e,p;
{
	register rec_ptr n;

	n = p->next;
	e->next = n;
	n->prev = e;
	p->next = e;
	e->prev = p;
}

/******************************************************************************\
|Routine: remq
|Callby: buffer_empty inquire killer rec_insert rec_merge rec_trim toss_data
|Purpose: Removes a record from a record queue.
|Arguments:
|    e is the record to remove.
\******************************************************************************/
void remq(e)
register rec_ptr e;
{
	register rec_ptr a,b;

	a = e->prev;
	b = e->next;
	a->next = b;
	b->prev = a;
}

