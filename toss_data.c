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

#include <stdlib.h>

#include "rec.h"

/******************************************************************************\
|Routine: toss_data
|Callby: main remove_win
|Purpose: Frees data associated with all records in a record queue.
|Arguments:
|    base is the base of the record queue.
\******************************************************************************/
void toss_data(base)
register rec_ptr base;
{
	rec_ptr cur;

	while(base->next != base)
/*	while(base->next != (rec_ptr)&base->next)*/
	{
		remq(cur = base->next);
		if(cur->recflags & 1)
			ifree(cur->data);
		ifree(cur);
	}
	ifree(base);
}

