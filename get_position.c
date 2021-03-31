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
|Routine: get_position
|Callby: command edit show_param
|Purpose: Returns the record number and byte offset from the beginning of the
|         file of the cursor location.
|Arguments:
|    record is the return record number.
|    byte is the returned byte offset from beginning of file.
\******************************************************************************/
void get_position(record,byte)
Int *record;
Int *byte;
{
	register rec_ptr r;
	Int bc,rc;

	for(r = BASE->next,rc = bc = 0;r != CURREC;r = r->next)
	{
		rc++;
		bc += r->length + 1;
	}
	*record = ++rc;
	*byte = ++bc + CURBYT;
}

