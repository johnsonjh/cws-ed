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

#include "memory.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: get_colbyt
|Callby: copier insert killer
|Purpose: Returns the byte offset in a record of the byte that corresponds to the given column number.
|         Converts all TABs in the record to spaces, and makes sure the record is long enough to include the given column.
|Arguments:
|    r is the record being analyzed.
|    column is the column number.
\******************************************************************************/
Int get_colbyt(r,column)
rec_ptr r;
Int column;
{
	register Int l;
	Char *buf;

	l = express(r->length,r->data,TAB_STRING[CUR_TAB_SETUP],TAB_LENGTH[CUR_TAB_SETUP],&buf,0,0);
	while(l < column)	/* this is dangerous */
		buf[l++] = ' ';
	if(r->recflags & 1)
		ifree(r->data);	/* replace the record with the expressed record */
	r->data = (Char *)imalloc(l + 1);
	r->recflags = 1;	/* mark it as freeable */
	memcpy(r->data,buf,l);
	r->data[l] = '\0';
	r->length = l;
	return(column - 1);	/* return trapped byte offset */
}

