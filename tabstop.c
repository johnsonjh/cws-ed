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
|Routine: tabstop
|Callby: find_column get_column inquire right_arrow
|Purpose: Returns the column number of the next tab stop following a given column.
|Arguments:
|    col is the column to start at.
\******************************************************************************/
Int tabstop(col)
register Int col;
{
	register Int i,cur,lim;
	register Char *c;

	cur = CUR_TAB_SETUP;
	lim = TAB_LENGTH[cur];
	if((i = col) >= lim)
		return(col - (col & 7) + 8);
	c = TAB_STRING[cur];
	do
	{
		i++;
	} while(c[i] != 't' && i < lim);
	return(i);
}

