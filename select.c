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

#include "ctyp_dec.h"
#include "rec.h"
#include "window.h"
#include "ed_dec.h"

/******************************************************************************\
|Routine: doselect
|Callby: edit paint paint_window
|Purpose: Displays the select marker.
|Arguments:
|    none
\******************************************************************************/
void doselect(r,byt)
rec_ptr r;
Int byt;
{
	Uchar b[2];

	reverse();
	if(r == BASE)
		b[0] = get_eob_char();
	else if(byt == r->length)
		b[0] = ' ';
	else if(HEXMODE)
		b[0] = '<';
	else if((b[0] = r->data[byt]) == '\t')
		b[0] = ' ';
	else if(NONPRINTNCS(b[0]))
		b[0] = '<';
	b[1] = '\0';
	putz((Char *)b);
	normal();
}

