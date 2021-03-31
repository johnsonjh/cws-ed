/*
 * Copyright (C) 1992 by Rush Record
 * Copyright (C) 1993 by Charles Sandmann (sandmann@clio.rice.edu)
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
|Routine: find_column
|Callby: down_arrow up_arrow
|Purpose: Returns the column number of the end of a record, and the length of
|         the data that fit on the screen.
|Arguments:
|    r is the record.
|    col is the column limit for the analysis of the record.
|    byte is the returned length of the data that fits on the screen.
\******************************************************************************/
Int find_column(r,col,byte)
register rec_ptr r;
register Int col,*byte;
{
	register Int i,c,last,ch;

	if(HEXMODE)
	{
		for(last = c = i = 0;i < r->length;i++)
		{
			last = c;
			c += 4;
			if(c >= col)
				break;
		}
	}
	else
	{
		for(last = c = i = 0;i < r->length;i++)
		{
			last = c;
			if((ch = r->data[i]) == '\t')
				c = tabstop(c);
			else if(NONPRINTNCS(ch))
				c += 4;
			else
				c++;
			if(c >= col)
				break;
		}
	}
	*byte = i;
	return(last + 1);
}

