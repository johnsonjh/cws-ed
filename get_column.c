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
|Routine: get_column
|Callby: cfrendly change_case copier down_arrow edit fix_scroll insert killer left_arrow match_paren move_eol move_word paint paint_window set_param trim unselect up_arrow word_fill
|Purpose: Returns the column number of a particular byte in a record.
|Arguments:
|    r is the record being analyzed.
|    byte is the byte offset in that record.
\******************************************************************************/
Int get_column(r,byte)
rec_ptr r;
Int byte;
{
	register Int c,ch;
	register Char *p,*q;

	if(HEXMODE)
		return((byte << 2) + 1);
	for(p = r->data,q = r->data + byte,c = 0;p != q;p++)
	{
		if((ch = *p) == '\t')
			c = tabstop(c);
		else if(NONPRINTNCS(ch))
			c += 4;
		else
			c++;
	}
	return(c + 1);
}

